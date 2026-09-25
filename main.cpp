#include "crow_all.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <array>

using namespace std;

string SUPABASE_URL;
string SUPABASE_KEY;

// ===== 쉘 명령 실행 =====
string execCmd(const string& cmd) {
    string result;
    array<char, 4096> buffer;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) result += buffer.data();
    pclose(pipe);
    return result;
}

string writeTempJson(const string& json) {
    char templ[] = "/tmp/sbXXXXXX";
    int fd = mkstemp(templ);
    if (fd == -1) return "";
    FILE* f = fdopen(fd, "w");
    fwrite(json.data(), 1, json.size(), f);
    fclose(f);
    return string(templ);
}

string urlEncode(const string& s) {
    string result;
    char buf[4];
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else {
            snprintf(buf, sizeof(buf), "%%%02X", c);
            result += buf;
        }
    }
    return result;
}

string sbGet(const string& pathWithQuery) {
    string cmd = "curl -s \"" + SUPABASE_URL + "/rest/v1/" + pathWithQuery + "\" "
        "-H \"apikey: " + SUPABASE_KEY + "\" "
        "-H \"Authorization: Bearer " + SUPABASE_KEY + "\"";
    return execCmd(cmd);
}

string sbPost(const string& path, const string& jsonBody) {
    string tmp = writeTempJson(jsonBody);
    string cmd = "curl -s -X POST \"" + SUPABASE_URL + "/rest/v1/" + path + "\" "
        "-H \"apikey: " + SUPABASE_KEY + "\" "
        "-H \"Authorization: Bearer " + SUPABASE_KEY + "\" "
        "-H \"Content-Type: application/json\" "
        "-H \"Prefer: return=representation\" "
        "--data-binary @" + tmp;
    string res = execCmd(cmd);
    remove(tmp.c_str());
    return res;
}

string sbPatch(const string& pathWithQuery, const string& jsonBody) {
    string tmp = writeTempJson(jsonBody);
    string cmd = "curl -s -X PATCH \"" + SUPABASE_URL + "/rest/v1/" + pathWithQuery + "\" "
        "-H \"apikey: " + SUPABASE_KEY + "\" "
        "-H \"Authorization: Bearer " + SUPABASE_KEY + "\" "
        "-H \"Content-Type: application/json\" "
        "-H \"Prefer: return=representation\" "
        "--data-binary @" + tmp;
    string res = execCmd(cmd);
    remove(tmp.c_str());
    return res;
}

string sbDelete(const string& pathWithQuery) {
    string cmd = "curl -s -X DELETE \"" + SUPABASE_URL + "/rest/v1/" + pathWithQuery + "\" "
        "-H \"apikey: " + SUPABASE_KEY + "\" "
        "-H \"Authorization: Bearer " + SUPABASE_KEY + "\"";
    return execCmd(cmd);
}

// ===== 구조체 =====
struct Member {
    string name;
    string birthdate;
    int points;
    int skillLevel;
    bool isAdmin;
};

struct Match {
    string date;
    string time;
    string location;
};

struct MvpRecord {
    int year;
    string memberName;
};

struct VideoLink {
    string date;
    string url;
};

struct Notice {
    int id;
    string date;
    string content;
};

// ===== 조회 =====
vector<Member> loadMembers() {
    vector<Member> members;
    auto parsed = crow::json::load(sbGet("members?select=*"));
    if (!parsed) return members;
    for (size_t i = 0; i < parsed.size(); i++) {
        Member m;
        m.name = parsed[i]["name"].s();
        m.birthdate = parsed[i]["birthdate"].s();
        m.points = parsed[i]["points"].i();
        m.skillLevel = parsed[i]["skill_level"].i();
        m.isAdmin = parsed[i]["is_admin"].b();
        members.push_back(m);
    }
    return members;
}

vector<Match> loadMatches() {
    vector<Match> matches;
    auto parsed = crow::json::load(sbGet("matches?select=*"));
    if (!parsed) return matches;
    for (size_t i = 0; i < parsed.size(); i++) {
        Match m;
        m.date = parsed[i]["date"].s();
        m.time = parsed[i]["time"].s();
        m.location = parsed[i]["location"].s();
        matches.push_back(m);
    }
    return matches;
}

vector<MvpRecord> loadMvpRecords() {
    vector<MvpRecord> records;
    auto parsed = crow::json::load(sbGet("mvp_records?select=*&order=year.desc"));
    if (!parsed) return records;
    for (size_t i = 0; i < parsed.size(); i++) {
        MvpRecord r;
        r.year = parsed[i]["year"].i();
        r.memberName = parsed[i]["member_name"].s();
        records.push_back(r);
    }
    return records;
}

vector<VideoLink> loadVideoLinks(int limit) {
    vector<VideoLink> links;
    string q = "video_links?select=*&order=id.desc";
    if (limit > 0) q += "&limit=" + to_string(limit);
    auto parsed = crow::json::load(sbGet(q));
    if (!parsed) return links;
    for (size_t i = 0; i < parsed.size(); i++) {
        VideoLink v;
        v.date = parsed[i]["date"].s();
        v.url = parsed[i]["url"].s();
        links.push_back(v);
    }
    return links;
}

vector<Notice> loadNotices() {
    vector<Notice> notices;
    auto parsed = crow::json::load(sbGet("notices?select=*&order=id.desc"));
    if (!parsed) return notices;
    for (size_t i = 0; i < parsed.size(); i++) {
        Notice n;
        n.id = parsed[i]["id"].i();
        n.date = parsed[i]["date"].s();
        n.content = parsed[i]["content"].s();
        notices.push_back(n);
    }
    return notices;
}

// ===== 유틸 =====
int findMemberIndex(vector<Member>& members, string name) {
    for (int i = 0; i < (int)members.size(); i++) {
        if (members[i].name == name) return i;
    }
    return -1;
}

bool compareBySkill(Member a, Member b) {
    return a.skillLevel > b.skillLevel;
}

string getToday() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    int year = 1900 + ltm->tm_year;
    int month = 1 + ltm->tm_mon;
    int day = ltm->tm_mday;
    string result = to_string(year) + "-";
    if (month < 10) result += "0";
    result += to_string(month) + "-";
    if (day < 10) result += "0";
    result += to_string(day);
    return result;
}

Match findNextMatch(vector<Match>& matches, string today) {
    Match closest;
    closest.date = "9999-99-99";
    for (int i = 0; i < (int)matches.size(); i++) {
        if (matches[i].date >= today && matches[i].date < closest.date) {
            closest = matches[i];
        }
    }
    return closest;
}

void addCors(crow::response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// ===== 초기 1회 시딩 (테이블이 비어있을 때만, 기존 txt 파일 내용으로 채움) =====
bool tableEmpty(const string& table) {
    auto parsed = crow::json::load(sbGet(table + "?select=id&limit=1"));
    if (!parsed) return false;
    return parsed.size() == 0;
}

void seedMembers() {
    if (!tableEmpty("members")) return;
    ifstream inFile("members.txt");
    if (!inFile) return;
    string line;
    crow::json::wvalue arr = crow::json::wvalue::list();
    int idx = 0;
    while (getline(inFile, line)) {
        stringstream ss(line);
        string name, birthdate, temp;
        getline(ss, name, ',');
        getline(ss, birthdate, ',');
        getline(ss, temp, ','); int points = temp.empty() ? 0 : stoi(temp);
        getline(ss, temp, ','); int skill = temp.empty() ? 1 : stoi(temp);
        getline(ss, temp, ','); bool isAdmin = (!temp.empty() && temp[0] == '1');
        crow::json::wvalue row;
        row["name"] = name;
        row["birthdate"] = birthdate;
        row["points"] = points;
        row["skill_level"] = skill;
        row["is_admin"] = isAdmin;
        arr[idx++] = move(row);
    }
    if (idx > 0) sbPost("members", arr.dump());
}

void seedMatches() {
    if (!tableEmpty("matches")) return;
    ifstream inFile("matches.txt");
    if (!inFile) return;
    string line;
    crow::json::wvalue arr = crow::json::wvalue::list();
    int idx = 0;
    while (getline(inFile, line)) {
        stringstream ss(line);
        string date, time_, location;
        getline(ss, date, ',');
        getline(ss, time_, ',');
        getline(ss, location, ',');
        crow::json::wvalue row;
        row["date"] = date;
        row["time"] = time_;
        row["location"] = location;
        arr[idx++] = move(row);
    }
    if (idx > 0) sbPost("matches", arr.dump());
}

void seedMvp() {
    if (!tableEmpty("mvp_records")) return;
    ifstream inFile("mvp_history.txt");
    if (!inFile) return;
    string line;
    crow::json::wvalue arr = crow::json::wvalue::list();
    int idx = 0;
    while (getline(inFile, line)) {
        stringstream ss(line);
        string temp, name;
        getline(ss, temp, ',');
        getline(ss, name, ',');
        if (temp.empty()) continue;
        crow::json::wvalue row;
        row["year"] = stoi(temp);
        row["member_name"] = name;
        arr[idx++] = move(row);
    }
    if (idx > 0) sbPost("mvp_records", arr.dump());
}

void seedVideos() {
    if (!tableEmpty("video_links")) return;
    ifstream inFile("video_links.txt");
    if (!inFile) return;
    string line;
    crow::json::wvalue arr = crow::json::wvalue::list();
    int idx = 0;
    while (getline(inFile, line)) {
        stringstream ss(line);
        string date, url;
        getline(ss, date, ',');
        getline(ss, url, ',');
        crow::json::wvalue row;
        row["date"] = date;
        row["url"] = url;
        arr[idx++] = move(row);
    }
    if (idx > 0) sbPost("video_links", arr.dump());
}

int main() {
    const char* url = getenv("SUPABASE_URL");
    const char* key = getenv("SUPABASE_KEY");
    SUPABASE_URL = url ? url : "";
    SUPABASE_KEY = key ? key : "";

    seedMembers();
    seedMatches();
    seedMvp();
    seedVideos();

    crow::SimpleApp app;
    srand(time(0));

    CROW_ROUTE(app, "/")([](){
        crow::response res("OMT 서버가 작동중입니다!");
        res.set_header("Content-Type", "text/plain; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 로그인 =====
    CROW_ROUTE(app, "/login").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string inputName = body["name"].s();
        string inputBirth = body["birthdate"].s();
        vector<Member> members = loadMembers();

        for (int i = 0; i < (int)members.size(); i++) {
            if (members[i].name == inputName && members[i].birthdate == inputBirth) {
                crow::json::wvalue result;
                result["success"] = true;
                result["name"] = members[i].name;
                result["points"] = members[i].points;
                result["skillLevel"] = members[i].skillLevel;
                result["isAdmin"] = members[i].isAdmin;
                crow::response res(result);
                res.set_header("Content-Type", "application/json; charset=utf-8");
                addCors(res);
                return res;
            }
        }
        crow::json::wvalue fail; fail["success"] = false;
        crow::response res(404, fail);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 관리자 비밀번호 확인 =====
    CROW_ROUTE(app, "/admin/verify").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }
        string password = body["password"].s();
        crow::json::wvalue result;
        result["success"] = (password == "omt_forever");
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 다음 매치 =====
    CROW_ROUTE(app, "/nextmatch").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        vector<Match> matches = loadMatches();
        Match next = findNextMatch(matches, getToday());
        crow::json::wvalue result;
        result["date"] = next.date;
        result["time"] = next.time;
        result["location"] = next.location;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 매치 등록 =====
    CROW_ROUTE(app, "/match/add").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        crow::json::wvalue row;
        row["date"] = body["date"].s();
        row["time"] = body["time"].s();
        row["location"] = body["location"].s();
        string arrBody = "[" + row.dump() + "]";
        sbPost("matches", arrBody);

        crow::json::wvalue result; result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 명예의 전당 조회 =====
    CROW_ROUTE(app, "/mvp").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        vector<MvpRecord> records = loadMvpRecords();
        crow::json::wvalue result;
        result["records"] = crow::json::wvalue::list();
        for (int i = 0; i < (int)records.size(); i++) {
            result["records"][i]["year"] = records[i].year;
            result["records"][i]["name"] = records[i].memberName;
        }
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== MVP 등록 =====
    CROW_ROUTE(app, "/mvp/add").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            fail["message"] = "관리자 비밀번호가 틀렸습니다.";
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        crow::json::wvalue row;
        row["year"] = body["year"].i();
        row["member_name"] = body["name"].s();
        string arrBody = "[" + row.dump() + "]";
        sbPost("mvp_records", arrBody);

        crow::json::wvalue result; result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 경기 영상 조회 (최신 5개) =====
    CROW_ROUTE(app, "/videos").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        vector<VideoLink> links = loadVideoLinks(5);
        crow::json::wvalue result;
        result["videos"] = crow::json::wvalue::list();
        for (int i = 0; i < (int)links.size(); i++) {
            result["videos"][i]["date"] = links[i].date;
            result["videos"][i]["url"] = links[i].url;
        }
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 경기 영상 등록 =====
    CROW_ROUTE(app, "/videos/add").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        crow::json::wvalue row;
        row["date"] = body["date"].s();
        row["url"] = body["url"].s();
        string arrBody = "[" + row.dump() + "]";
        sbPost("video_links", arrBody);

        crow::json::wvalue result; result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 포인트 수정 (증감/초기화) =====
    CROW_ROUTE(app, "/member/points").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        string targetName = body["name"].s();
        string mode = body["mode"].s();

        vector<Member> members = loadMembers();
        int idx = findMemberIndex(members, targetName);
        if (idx == -1) {
            crow::json::wvalue fail; fail["success"] = false;
            fail["message"] = "회원을 찾을 수 없습니다.";
            crow::response res(404, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        int newPoints;
        if (mode == "reset") {
            newPoints = 0;
        } else {
            int delta = body["value"].i();
            newPoints = members[idx].points + delta;
        }

        crow::json::wvalue patch;
        patch["points"] = newPoints;
        sbPatch("members?name=eq." + urlEncode(targetName), patch.dump());

        crow::json::wvalue result; result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 회원 추가 =====
    CROW_ROUTE(app, "/member/add").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        crow::json::wvalue row;
        row["name"] = body["name"].s();
        row["birthdate"] = body["birthdate"].s();
        row["points"] = 0;
        row["skill_level"] = body["skillLevel"].i();
        row["is_admin"] = false;
        string arrBody = "[" + row.dump() + "]";
        sbPost("members", arrBody);

        crow::json::wvalue result; result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 회원 정보 수정 =====
    CROW_ROUTE(app, "/member/update").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        string targetName = body["name"].s();
        vector<Member> members = loadMembers();
        int idx = findMemberIndex(members, targetName);
        if (idx == -1) {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(404, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        crow::json::wvalue patch;
        bool hasChange = false;
        if (body.has("birthdate")) { patch["birthdate"] = body["birthdate"].s(); hasChange = true; }
        if (body.has("skillLevel")) { patch["skill_level"] = body["skillLevel"].i(); hasChange = true; }

        if (hasChange) {
            sbPatch("members?name=eq." + urlEncode(targetName), patch.dump());
        }

        crow::json::wvalue result; result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 회원 삭제 =====
    CROW_ROUTE(app, "/member/delete").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        string targetName = body["name"].s();
        vector<Member> members = loadMembers();
        int idx = findMemberIndex(members, targetName);
        if (idx == -1) {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(404, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        sbDelete("members?name=eq." + urlEncode(targetName));

        crow::json::wvalue result; result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 회원 전체 목록 (이름/레벨/포인트만, 생년월일 제외) =====
    CROW_ROUTE(app, "/member/list").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        vector<Member> members = loadMembers();
        sort(members.begin(), members.end(), [](const Member& a, const Member& b){
            return a.points > b.points;
        });

        crow::json::wvalue result;
        result["members"] = crow::json::wvalue::list();
        for (int i = 0; i < (int)members.size(); i++) {
            result["members"][i]["name"] = members[i].name;
            result["members"][i]["skillLevel"] = members[i].skillLevel;
            result["members"][i]["points"] = members[i].points;
        }
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 팀 자동 배정 =====
    CROW_ROUTE(app, "/team/assign").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        vector<Member> members = loadMembers();
        vector<Member> attendees;

        auto names = body["names"];
        for (size_t i = 0; i < names.size(); i++) {
            string name = names[i].s();
            for (size_t j = 0; j < members.size(); j++) {
                if (members[j].name == name) { attendees.push_back(members[j]); break; }
            }
        }

        sort(attendees.begin(), attendees.end(), compareBySkill);

        for (size_t i = 0; i < attendees.size(); i++) {
            size_t j = i;
            while (j < attendees.size() && attendees[j].skillLevel == attendees[i].skillLevel) j++;
            for (size_t k = j - 1; k > i; k--) {
                size_t r = i + rand() % (k - i + 1);
                swap(attendees[k], attendees[r]);
            }
            i = j - 1;
        }

        vector<Member> teamA, teamB;
        for (size_t i = 0; i < attendees.size(); i++) {
            if (i % 2 == 0) teamA.push_back(attendees[i]);
            else teamB.push_back(attendees[i]);
        }

        crow::json::wvalue result;
        result["teamA"] = crow::json::wvalue::list();
        for (size_t i = 0; i < teamA.size(); i++) result["teamA"][i] = teamA[i].name;
        result["teamB"] = crow::json::wvalue::list();
        for (size_t i = 0; i < teamB.size(); i++) result["teamB"][i] = teamB[i].name;

        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 공지사항 조회 =====
    CROW_ROUTE(app, "/notices").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        vector<Notice> notices = loadNotices();
        crow::json::wvalue result;
        result["notices"] = crow::json::wvalue::list();
        for (int i = 0; i < (int)notices.size(); i++) {
            result["notices"][i]["id"] = notices[i].id;
            result["notices"][i]["date"] = notices[i].date;
            result["notices"][i]["content"] = notices[i].content;
        }
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 공지사항 등록 =====
    CROW_ROUTE(app, "/notices/add").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        crow::json::wvalue row;
        row["date"] = body["date"].s();
        row["content"] = body["content"].s();
        string arrBody = "[" + row.dump() + "]";
        sbPost("notices", arrBody);

        crow::json::wvalue result; result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 공지사항 삭제 =====
    CROW_ROUTE(app, "/notices/delete").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204); addCors(res); return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) { crow::response res(400); addCors(res); return res; }

        string password = body["password"].s();
        if (password != "omt_forever") {
            crow::json::wvalue fail; fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res); return res;
        }

        int id = body["id"].i();
        sbDelete("notices?id=eq." + to_string(id));

        crow::json::wvalue result; result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    app.port(18080).multithreaded().run();
    return 0;
}
