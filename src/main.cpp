#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ncurses.h>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>

struct Object {int x, y, width; long long last_time;};

auto start = std::chrono::steady_clock::now();

long long Get_Ticks() {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    ).count();
    return elapsed;
}

long get_record() {
    long record;
    std::ifstream ifs("./record");
    if (!ifs) return 0;

    std::string line;
    if (std::getline(ifs, line)) {
        if (!std::all_of(line.begin(), line.end(), ::isdigit)) {
            std::remove("./record");
            return 0;
        }
        record = std::stoi(line);
    } else return 0;

    return record;
}

void set_record(long record) {
    std::ofstream ofs("./record", std::ios::out | std::ios::trunc);
    if (ofs.is_open()) {
        ofs << record;
        ofs.close();
    }
}

int main() {
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);
    srand(time(NULL));

    init_pair(1, COLOR_BLUE, COLOR_WHITE);
    init_pair(2, COLOR_RED, COLOR_YELLOW);
    init_pair(3, COLOR_CYAN, COLOR_MAGENTA);

    int ch;
    auto running = true;

    auto player_x = COLS / 2;
    auto player_y = 5;

    auto live = 3;
    auto record = 0L;
    auto record_max = get_record();

    auto render_obstacle_time = Get_Ticks() - 3000;
    auto render_point_time = Get_Ticks();
    auto record_time = Get_Ticks();
    auto advanced_time = Get_Ticks();

    auto timeout = 3000;
    auto velocity = 500;

    auto points_bonus = 20;

    std::vector<Object> obstacles;
    std::vector<Object> points;
    obstacles.push_back({COLS / 2, 100, (rand() % 11) + 8, Get_Ticks()});

    do {
        move(0, 0);
        clrtoeol();
        mvprintw(0, 0, "live: %d", live);
        mvprintw(0, COLS / 2, "record: %ld/%ld", record, record_max);

        mvaddch(5, player_x, ' ');
        int ch = getch();
        switch (ch) {
            case KEY_LEFT:
            player_x--;
            break;
            case KEY_RIGHT:
            player_x++;
            break;
            case 'q':
            case 'Q':
            running = false;
        }

        if (player_x < 0) player_x = 0;
        else if (player_x >= COLS) player_x = COLS - 1;

        attron(COLOR_PAIR(1));
        mvaddch(player_y, player_x, '@');
        attroff(COLOR_PAIR(1));

        auto current_time = Get_Ticks();
        if ((current_time - render_obstacle_time) >= timeout) {
            Object newObstacle;
            int width = (rand() % 11) + 8;
            int x = rand() % (COLS - width + 1);

            newObstacle.x = x;
            newObstacle.y = LINES - 1;
            newObstacle.width = width;
            newObstacle.last_time = current_time;

            obstacles.push_back(newObstacle);
            render_obstacle_time = current_time;
        }

        for (int i = 0; i < obstacles.size(); i++) {
            mvprintw(obstacles[i].y, obstacles[i].x, "%s", std::string(obstacles[i].width, ' ').c_str());
            if ((current_time - obstacles[i].last_time) >= velocity) {
                obstacles[i].y--;
                obstacles[i].last_time = current_time;
            }
            if (obstacles[i].y > 2) {
                attron(COLOR_PAIR(2));
                mvprintw(obstacles[i].y, obstacles[i].x, "%s", std::string(obstacles[i].width, '=').c_str());
                attroff(COLOR_PAIR(2));
            }
            if (obstacles[i].y <= 2) {
                obstacles.erase(obstacles.begin() + i);
                i--;
            }
        }

        if ((current_time - render_point_time) >= 2000) {
            points.push_back({rand() % (COLS), LINES - 1, 1, current_time});
            render_point_time = current_time;
        }

        for (int i = 0; i < points.size(); i++) {
            mvprintw(points[i].y, points[i].x, " ");
            if ((current_time - points[i].last_time) >= velocity) {
                points[i].y--;
                points[i].last_time = current_time;
            }
            if (points[i].y > 2) {
                attron(COLOR_PAIR(3));
                mvprintw(points[i].y, points[i].x, "#");
                attroff(COLOR_PAIR(3));
            }
            if (points[i].y <= 2) {
                points.erase(points.begin() + i);
                i--;
            }
        }

        if (live <= 0) {
            std::string go = "Game Over!";
            mvprintw(LINES / 2, (COLS / 2) - (go.length() / 2), "%s", go.c_str());
            if (record > get_record()) set_record(record);
            nodelay(stdscr, FALSE);
            ch = getch();
            while (ch != 'q')
                ch = getch();
            running = false;
        }

        for (int i = 0; i < obstacles.size(); i++) {
            if ((player_x >= obstacles[i].x && player_x <= (obstacles[i].x + (obstacles[i].width - 1))) && (obstacles[i].y == player_y)) {
                if (live > 0) live--;
                mvprintw(obstacles[i].y, obstacles[i].x, "%s", std::string(obstacles[i].width, ' ').c_str());
                obstacles.erase(obstacles.begin() + i);
            }
        }

        for (int i = 0; i < points.size(); i++) {
            if (player_x == points[i].x && points[i].y == player_y) {
                mvprintw(points[i].y, points[i].x, " ");
                points.erase(points.begin() + i);
                if (live > 0) record += points_bonus;
            }
        }

        if ((current_time - record_time) >= 600 && live > 0) {
            record++;
            if (record > record_max) record_max = record;
            record_time = current_time;
        }
        if ((current_time - advanced_time) >= 300) {
            if (timeout > 500) timeout -= 15;
            if (velocity > 50) velocity -= 2;
            advanced_time = current_time;
        }
        refresh();
        napms(10);
    } while (running);

    endwin();
}
