#include "stellar_todo.h"

#include "mcp_server.h"
#include "settings.h"
#include <cJSON.h>
#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

namespace stellar_max35 {
namespace {
struct Todo { std::string text; std::string when; };
constexpr int kMaxItems = 8;

// Product UI refreshes frequently. Keep the working set in RAM so rendering the
// home page never causes repeated NVS reads. NVS is touched only on first load
// and when a memo is changed.
std::mutex g_mutex;
std::vector<Todo> g_items;
bool g_loaded = false;

std::vector<Todo> LoadFromNvs() {
    Settings s("stellar_todo", false);
    std::string raw = s.GetString("items");
    std::vector<Todo> out;
    if (raw.empty()) return out;
    cJSON* root = cJSON_Parse(raw.c_str());
    if (!cJSON_IsArray(root)) {
        if (root) cJSON_Delete(root);
        return out;
    }
    cJSON* item = nullptr;
    cJSON_ArrayForEach(item, root) {
        auto* t = cJSON_GetObjectItem(item, "text");
        auto* w = cJSON_GetObjectItem(item, "when");
        if (cJSON_IsString(t) && t->valuestring && t->valuestring[0]) {
            out.push_back({t->valuestring, cJSON_IsString(w) && w->valuestring ? w->valuestring : ""});
            if (static_cast<int>(out.size()) >= kMaxItems) break;
        }
    }
    cJSON_Delete(root);
    return out;
}

void EnsureLoadedLocked() {
    if (!g_loaded) {
        g_items = LoadFromNvs();
        g_loaded = true;
    }
}

void SaveLocked() {
    cJSON* a = cJSON_CreateArray();
    if (!a) return;
    for (const auto& i : g_items) {
        cJSON* o = cJSON_CreateObject();
        if (!o) continue;
        cJSON_AddStringToObject(o, "text", i.text.c_str());
        cJSON_AddStringToObject(o, "when", i.when.c_str());
        cJSON_AddItemToArray(a, o);
    }
    char* p = cJSON_PrintUnformatted(a);
    if (p) {
        Settings s("stellar_todo", true);
        s.SetString("items", p);
        cJSON_free(p);
    }
    cJSON_Delete(a);
}

std::string JsonListLocked() {
    cJSON* a = cJSON_CreateArray();
    if (!a) return "[]";
    for (int n = 0; n < static_cast<int>(g_items.size()); ++n) {
        cJSON* o = cJSON_CreateObject();
        if (!o) continue;
        cJSON_AddNumberToObject(o, "index", n + 1);
        cJSON_AddStringToObject(o, "text", g_items[n].text.c_str());
        cJSON_AddStringToObject(o, "when", g_items[n].when.c_str());
        cJSON_AddItemToArray(a, o);
    }
    char* p = cJSON_PrintUnformatted(a);
    std::string r = p ? p : "[]";
    if (p) cJSON_free(p);
    cJSON_Delete(a);
    return r;
}

std::string CleanText(const std::string& in, size_t max_len) {
    if (in.size() <= max_len) return in;
    size_t end = max_len;
    // Never persist an invalid UTF-8 tail when applying the storage ceiling.
    while (end > 0 && (static_cast<unsigned char>(in[end]) & 0xC0) == 0x80) --end;
    if (end == 0) return {};
    const unsigned char lead = static_cast<unsigned char>(in[end]);
    size_t need = 1;
    if ((lead & 0xE0) == 0xC0) need = 2;
    else if ((lead & 0xF0) == 0xE0) need = 3;
    else if ((lead & 0xF8) == 0xF0) need = 4;
    if (end + need > max_len) return in.substr(0, end);
    return in.substr(0, max_len);
}
}  // namespace

std::string GetTodayMemoText() {
    std::lock_guard<std::mutex> lock(g_mutex);
    EnsureLoadedLocked();
    if (g_items.empty()) return "暂无备忘\n对我说：添加待办……";
    std::string out;
    const int count = std::min<int>(3, g_items.size());
    for (int i = 0; i < count; ++i) {
        if (i) out += "\n";
        out += "• ";
        if (!g_items[i].when.empty()) {
            out += g_items[i].when;
            out += "  ";
        }
        out += g_items[i].text;
    }
    if (static_cast<int>(g_items.size()) > count) out += "\n…还有更多备忘";
    return out;
}

void RegisterTodoTools() {
    auto& m = McpServer::GetInstance();
    m.AddTool("self.stellar.todo.list", "列出设备本地备忘录。", PropertyList(),
        [](const PropertyList&) -> ReturnValue {
            std::lock_guard<std::mutex> lock(g_mutex);
            EnsureLoadedLocked();
            return JsonListLocked();
        });

    m.AddTool("self.stellar.todo.add",
        "添加一条设备本地备忘。内容保持简短。when 是用于桌面显示的简短时间文本，可传空字符串。",
        PropertyList({Property("text", kPropertyTypeString), Property("when", kPropertyTypeString)}),
        [](const PropertyList& p) -> ReturnValue {
            const std::string text = CleanText(p["text"].value<std::string>(), 160);
            const std::string when = CleanText(p["when"].value<std::string>(), 48);
            if (text.empty()) return false;
            std::lock_guard<std::mutex> lock(g_mutex);
            EnsureLoadedLocked();
            if (static_cast<int>(g_items.size()) >= kMaxItems) {
                return std::string("备忘录已满，请先删除一条");
            }
            g_items.push_back({text, when});
            SaveLocked();
            return true;
        });

    m.AddTool("self.stellar.todo.remove", "按序号删除设备本地备忘。",
        PropertyList({Property("index", kPropertyTypeInteger, 1, kMaxItems)}),
        [](const PropertyList& p) -> ReturnValue {
            std::lock_guard<std::mutex> lock(g_mutex);
            EnsureLoadedLocked();
            const int idx = p["index"].value<int>() - 1;
            if (idx < 0 || idx >= static_cast<int>(g_items.size())) return false;
            g_items.erase(g_items.begin() + idx);
            SaveLocked();
            return true;
        });

    m.AddTool("self.stellar.todo.clear", "清空设备本地备忘录。", PropertyList(),
        [](const PropertyList&) -> ReturnValue {
            std::lock_guard<std::mutex> lock(g_mutex);
            EnsureLoadedLocked();
            g_items.clear();
            SaveLocked();
            return true;
        });
}

}  // namespace stellar_max35
