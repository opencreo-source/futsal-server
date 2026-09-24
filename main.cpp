#include "crow_all.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
#include <cstdlib>

using namespace std;

struct Member {
    string name;
    string birthdate;
    int points;
    int skillLevel;
    bool isAdmin;
};

vector<Member> loadMembers() {
    vector<Member> members;
    ifstream inFile("members.txt");
    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        Member m;
        string temp;
        getline(ss, m.name, ',');
        getline(ss, m.birthdate, ',');
        getline(ss, temp, ',');
        m.points = stoi(temp);
        getline(ss, temp, ',');
        m.skillLevel = stoi(temp);
        getline(ss, temp, ',');
        m.isAdmin = (temp[0] == '1');
        members.push_back(m);
    }
    return members;
}

void saveMembers(vector<Member>& members) {
    ofstream outFile("members.txt");
    for (int i = 0; i < members.size(); i++) {
        outFile << members[i].name << ","
                << members[i].birthdate << ","
                << members[i].points << ","
                << members[i].skillLevel << ","
                << members[i].isAdmin << endl;
    }
    outFile.close();
}

int findMemberIndex(vector<Member>& members, string name) {
    for (int i = 0; i < members.size(); i++) {
        if (members[i].name == name) return i;
    }
    return -1;
}

bool compareBySkill(Member a, Member b) {
    return a.skillLevel > b.skillLevel;
}

struct Match {
    string date;
    string time;
    string location;
};

vector<Match> loadMatches() {
    vector<Match> matches;
    ifstream inFile("matches.txt");
    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        Match m;
        getline(ss, m.date, ',');
        getline(ss, m.time, ',');
        getline(ss, m.location, ',');
        matches.push_back(m);
    }
    return matches;
}

void saveMatches(vector<Match>& matches) {
    ofstream outFile("matches.txt");
    for (int i = 0; i < matches.size(); i++) {
        outFile << matches[i].date << "," << matches[i].time << "," << matches[i].location << endl;
    }
    outFile.close();
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
    for (int i = 0; i < matches.size(); i++) {
        if (matches[i].date >= today && matches[i].date < closest.date) {
            closest = matches[i];
        }
    }
    return closest;
}

struct MvpRecord {
    int year;
    string memberName;
};

vector<MvpRecord> loadMvpRecords() {
    vector<MvpRecord> records;
    ifstream inFile("mvp_history.txt");
    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        MvpRecord r;
        string temp;
        getline(ss, temp, ',');
        r.year = stoi(temp);
        getline(ss, r.memberName, ',');
        records.push_back(r);
    }
    return records;
}

void saveMvpRecords(vector<MvpRecord>& records) {
    ofstream outFile("mvp_history.txt");
    for (int i = 0; i < records.size(); i++) {
        outFile << records[i].year << "," << records[i].memberName << endl;
    }
    outFile.close();
}

struct VideoLink {
    string date;
    string url;
};

vector<VideoLink> loadVideoLinks() {
    vector<VideoLink> links;
    ifstream inFile("video_links.txt");
    string line;
    while (getline(inFile, line)) {
        stringstream ss(line);
        VideoLink v;
        getline(ss, v.date, ',');
        getline(ss, v.url, ',');
        links.push_back(v);
    }
    return links;
}

void saveVideoLinks(vector<VideoLink>& links) {
    ofstream outFile("video_links.txt");
    for (int i = 0; i < links.size(); i++) {
        outFile << links[i].date << "," << links[i].url << endl;
    }
    outFile.close();
}

// ===== CORS 허용 헤더를 모든 응답에 붙여주는 함수 =====
void addCors(crow::response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
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
            crow::response res(204);
            addCors(res);
            return res;
        }

        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400);
            addCors(res);
            return res;
        }

        string inputName = body["name"].s();
        string inputBirth = body["birthdate"].s();
        vector<Member> members = loadMembers();

        for (int i = 0; i < members.size(); i++) {
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

        crow::json::wvalue fail;
        fail["success"] = false;
        crow::response res(404, fail);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 다음 매치 =====
    CROW_ROUTE(app, "/nextmatch").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            addCors(res);
            return res;
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
        // ===== 관리자 비밀번호 확인 (부작용 없음) =====
    CROW_ROUTE(app, "/admin/verify").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            addCors(res);
            return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400);
            addCors(res);
            return res;
        }
        string password = body["password"].s();
        crow::json::wvalue result;
        result["success"] = (password == "futsal2026");
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });
    // ===== 매치 등록 =====
    CROW_ROUTE(app, "/match/add").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            addCors(res);
            return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400);
            addCors(res);
            return res;
        }

        string password = body["password"].s();
        if (password != "futsal2026") {
            crow::json::wvalue fail;
            fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        Match newMatch;
        newMatch.date = body["date"].s();
        newMatch.time = body["time"].s();
        newMatch.location = body["location"].s();

        vector<Match> matches = loadMatches();
        matches.push_back(newMatch);
        saveMatches(matches);

        crow::json::wvalue result;
        result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 명예의 전당 조회 =====
    CROW_ROUTE(app, "/mvp").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            addCors(res);
            return res;
        }
        vector<MvpRecord> records = loadMvpRecords();
        crow::json::wvalue result;
        result["records"] = crow::json::wvalue::list();
        for (int i = 0; i < records.size(); i++) {
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
            crow::response res(204);
            addCors(res);
            return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400);
            addCors(res);
            return res;
        }

        string password = body["password"].s();
        if (password != "futsal2026") {
            crow::json::wvalue fail;
            fail["success"] = false;
            fail["message"] = "관리자 비밀번호가 틀렸습니다.";
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        int year = body["year"].i();
        string name = body["name"].s();
        vector<MvpRecord> records = loadMvpRecords();
        MvpRecord newRecord;
        newRecord.year = year;
        newRecord.memberName = name;
        records.push_back(newRecord);
        saveMvpRecords(records);

        crow::json::wvalue result;
        result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 경기 영상 조회 =====
    CROW_ROUTE(app, "/videos").methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            addCors(res);
            return res;
        }
        vector<VideoLink> links = loadVideoLinks();
        crow::json::wvalue result;
        result["videos"] = crow::json::wvalue::list();
        int count = 0;
        int idx = 0;
        for (int i = (int)links.size() - 1; i >= 0 && count < 5; i--) {
            result["videos"][idx]["date"] = links[i].date;
            result["videos"][idx]["url"] = links[i].url;
            idx++;
            count++;
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
            crow::response res(204);
            addCors(res);
            return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400);
            addCors(res);
            return res;
        }

        string password = body["password"].s();
        if (password != "futsal2026") {
            crow::json::wvalue fail;
            fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        string date = body["date"].s();
        string url = body["url"].s();
        vector<VideoLink> links = loadVideoLinks();
        VideoLink newLink;
        newLink.date = date;
        newLink.url = url;
        links.push_back(newLink);
        saveVideoLinks(links);

        crow::json::wvalue result;
        result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 포인트 수정 =====
    CROW_ROUTE(app, "/member/points").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            addCors(res);
            return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400);
            addCors(res);
            return res;
        }

        string password = body["password"].s();
        if (password != "futsal2026") {
            crow::json::wvalue fail;
            fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        string targetName = body["name"].s();
        int newPoints = body["points"].i();
        vector<Member> members = loadMembers();
        int idx = findMemberIndex(members, targetName);

        if (idx == -1) {
            crow::json::wvalue fail;
            fail["success"] = false;
            fail["message"] = "회원을 찾을 수 없습니다.";
            crow::response res(404, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        members[idx].points = newPoints;
        saveMembers(members);

        crow::json::wvalue result;
        result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 회원 추가 =====
    CROW_ROUTE(app, "/member/add").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            addCors(res);
            return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400);
            addCors(res);
            return res;
        }

        string password = body["password"].s();
        if (password != "futsal2026") {
            crow::json::wvalue fail;
            fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        Member newMember;
        newMember.name = body["name"].s();
        newMember.birthdate = body["birthdate"].s();
        newMember.points = 0;
        newMember.skillLevel = body["skillLevel"].i();
        newMember.isAdmin = false;

        vector<Member> members = loadMembers();
        members.push_back(newMember);
        saveMembers(members);

        crow::json::wvalue result;
        result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 회원 정보 수정 =====
    CROW_ROUTE(app, "/member/update").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            addCors(res);
            return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400);
            addCors(res);
            return res;
        }

        string password = body["password"].s();
        if (password != "futsal2026") {
            crow::json::wvalue fail;
            fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        string targetName = body["name"].s();
        vector<Member> members = loadMembers();
        int idx = findMemberIndex(members, targetName);

        if (idx == -1) {
            crow::json::wvalue fail;
            fail["success"] = false;
            crow::response res(404, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        if (body.has("birthdate")) members[idx].birthdate = body["birthdate"].s();
        if (body.has("skillLevel")) members[idx].skillLevel = body["skillLevel"].i();
        saveMembers(members);

        crow::json::wvalue result;
        result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 회원 삭제 =====
    CROW_ROUTE(app, "/member/delete").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            addCors(res);
            return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400);
            addCors(res);
            return res;
        }

        string password = body["password"].s();
        if (password != "futsal2026") {
            crow::json::wvalue fail;
            fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        string targetName = body["name"].s();
        vector<Member> members = loadMembers();
        int idx = findMemberIndex(members, targetName);

        if (idx == -1) {
            crow::json::wvalue fail;
            fail["success"] = false;
            crow::response res(404, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        members.erase(members.begin() + idx);
        saveMembers(members);

        crow::json::wvalue result;
        result["success"] = true;
        crow::response res(result);
        res.set_header("Content-Type", "application/json; charset=utf-8");
        addCors(res);
        return res;
    });

    // ===== 팀 자동 배정 =====
    CROW_ROUTE(app, "/team/assign").methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            crow::response res(204);
            addCors(res);
            return res;
        }
        auto body = crow::json::load(req.body);
        if (!body) {
            crow::response res(400);
            addCors(res);
            return res;
        }

        string password = body["password"].s();
        if (password != "futsal2026") {
            crow::json::wvalue fail;
            fail["success"] = false;
            crow::response res(403, fail);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            addCors(res);
            return res;
        }

        vector<Member> members = loadMembers();
        vector<Member> attendees;

        auto names = body["names"];
        for (size_t i = 0; i < names.size(); i++) {
            string name = names[i].s();
            for (size_t j = 0; j < members.size(); j++) {
                if (members[j].name == name) {
                    attendees.push_back(members[j]);
                    break;
                }
            }
        }

        sort(attendees.begin(), attendees.end(), compareBySkill);

        for (size_t i = 0; i < attendees.size(); i++) {
            size_t j = i;
            while (j < attendees.size() && attendees[j].skillLevel == attendees[i].skillLevel) {
                j++;
            }
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

    app.port(18080).multithreaded().run();
    return 0;
}
