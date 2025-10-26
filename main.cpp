#include <ctime>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <sqlite3.h>

#include "raylib.h"

struct Task {
    int id;
    std::string description;
    bool done;
};

// TODO: Handle selection of the entire input box (delete word or phrase)
// TODO: Handle backspace press for a long period of time

#define COLOR_BG      CLITERAL(Color){ 30, 37, 45, 255 }
// #define COLOR_BG      CLITERAL(Color){ 220, 230, 235, 255 }
#define COLOR_ACCENT  CLITERAL(Color){ 100, 149, 237, 255 }
#define COLOR_TEXT    CLITERAL(Color){ 50, 50, 70, 255 }
#define COLOR_TASK_BG CLITERAL(Color){ 255, 255, 255, 255 }
#define COLOR_DONE    CLITERAL(Color){ 152, 251, 152, 255 }
#define COLOR_DELETE  CLITERAL(Color){ 255, 99, 71, 255 }

sqlite3 *db         = nullptr;
const char* DB_FILE = "todo_list.db";

const int screenWidth     = 800;
const int screenHeight    = 600;
const int MAX_INPUT_CHARS = 64;

char inputTaskText[MAX_INPUT_CHARS + 1] = "\0";
int letterCount        = 0;
bool isInputBoxActive  = false;
Rectangle inputTaskBox = { 50, 120, 550, 40 };
Rectangle addButton    = { 610, 120, 140, 40 };

double lastBackspaceTime  = 0.0;
const double initialDelay = 0.5;
const double repeatRate   = 0.05;
bool firstBackspace       = true;

std::vector<Task> tasks;

// Callback per sqlite3_exec
static int callback(void *NotUsed, int argc, char **argv, char **azColName) { return 0; }

void InitDatabase() {
    if (sqlite3_open(DB_FILE, &db)) {
        std::cerr << "[ERROR] " << sqlite3_errmsg(db) << std::endl;
        exit(1);
    }

    const char *sql = 
        "CREATE TABLE IF NOT EXISTS TASKS("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "DESCRIPTION TEXT NOT NULL,"
        "DONE INT NOT NULL DEFAULT 0);";

    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    } else {
        std::cout << "TASKS table alreay present." << std::endl;
    }
}

static int selectCallback(void *data, int argc, char **argv, char **azColName) {
    Task t;

    t.id          = argv[0] ? std::stoi(argv[0]) : 0;
    t.description = argv[1] ? argv[1] : "";
    t.done        = argv[2] ? (std::stoi(argv[2]) == 1) : false;
    static_cast<std::vector<Task>*>(data)->push_back(t);
    return 0;
}

void LoadTasks() {
    tasks.clear();

    const char *sql = "SELECT ID, DESCRIPTION, DONE FROM TASKS ORDER BY ID DESC;";
    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, sql, selectCallback, &tasks, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    }
}

void AddTask(const std::string& description) {
    if (description.empty()) return;

    std::string sql = "INSERT INTO TASKS (DESCRIPTION) VALUES ('" + description + "');";
    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    } else {
        LoadTasks();
    }
}

void UpdateTaskDone(int id, bool done) {
    std::string sql = "UPDATE TASKS SET DONE = " + std::to_string(done ? 1 : 0) + 
                    " WHERE ID = " + std::to_string(id) + ";";
    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    } else {
        LoadTasks();
    }
}

void DeleteTask(int id) {
    std::string sql = "DELETE FROM TASKS WHERE ID = " + std::to_string(id) + ";";
    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    } else {
        LoadTasks();
    }
}

// =================================================================
// -- UI (Raylib)
// =================================================================

void HandleInput() {
    // Mouse Click
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePoint = GetMousePosition();

        // Attiva/Disattiva l'input box
        if (CheckCollisionPointRec(mousePoint, inputTaskBox)) {
            isInputBoxActive = true;
        } else {
            isInputBoxActive = false;
            // TODO: ADD more logic
        }

        // Gestione click sul bottone AGGIUNGI
        if (CheckCollisionPointRec(mousePoint, addButton)) {
            if (letterCount > 0) {
                AddTask(std::string(inputTaskText));
                inputTaskText[0] = '\0';
                letterCount      = 0;
                isInputBoxActive = false;
            }
        }

        int taskY      = 200;
        int taskHeight = 50;
        int spacing    = 10;

        for (auto& task : tasks) {
            Rectangle taskRec         = { 50, (float)taskY, (float)screenWidth - 100, (float)taskHeight };
            Rectangle checkBoxRec     = { taskRec.x + 10, taskRec.y + 15, 20, 20 };
            Rectangle deleteButtonRec = { taskRec.x + taskRec.width - 100, taskRec.y + 10, 90, 30 };

            if (CheckCollisionPointRec(mousePoint, checkBoxRec)) {
                UpdateTaskDone(task.id, !task.done);
                return;
            }

            if (CheckCollisionPointRec(mousePoint, deleteButtonRec)) {
                DeleteTask(task.id);
                return;
            }

            taskY += taskHeight + spacing;
        }
    }

    // Keyboard input
    if (isInputBoxActive) {
        int key = GetCharPressed();

        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (letterCount < MAX_INPUT_CHARS)) {
                inputTaskText[letterCount] = (char)key;
                letterCount++;
                inputTaskText[letterCount] = '\0';
            }
            key = GetCharPressed();
        }

        // Backspace handling
        if (IsKeyDown(KEY_BACKSPACE)) {
            double currentTime = GetTime();
            bool shouldDelete  = false;
            
            if (firstBackspace) {
                shouldDelete = IsKeyPressed(KEY_BACKSPACE);
                if (shouldDelete) {
                    firstBackspace    = false;
                    lastBackspaceTime = currentTime;
                }
            } else {
                if (currentTime - lastBackspaceTime >= repeatRate) {
                    shouldDelete = true;
                    lastBackspaceTime = currentTime;
                }
            }

            if (shouldDelete && letterCount > 0) {
                letterCount--;
                inputTaskText[letterCount] = '\0';
            }
        } else if (IsKeyReleased(KEY_BACKSPACE)) {
            firstBackspace = true;
        }

        // ENTER handling
        if (IsKeyPressed(KEY_ENTER)) {
             if (letterCount > 0) {
                AddTask(std::string(inputTaskText));
                inputTaskText[0] = '\0';
                letterCount      = 0;
                isInputBoxActive = false;
            }
        }
    }
}

void DrawUI() {
    BeginDrawing();
    ClearBackground(COLOR_BG);

    DrawText("To-Do List (Raylib + SQLite)", 50, 40, 30, COLOR_ACCENT);
    DrawLine(50, 85, screenWidth - 50, 85, COLOR_ACCENT);

    time_t now = time(0);
    tm* ltm    = localtime(&now);

    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", ltm);

    int fontSize = 20, margin = 10, textWidth = MeasureText(timeStr, fontSize);
    int x = GetScreenWidth() - textWidth - margin;
    int y = GetScreenHeight() - fontSize - margin;

    DrawText(timeStr, x, y, fontSize, LIGHTGRAY);

    // 1. INPUT Box
    Color inputColor = isInputBoxActive ? LIGHTGRAY : COLOR_TASK_BG;
    DrawRectangleRec(inputTaskBox, inputColor);
    DrawRectangleLines((int)inputTaskBox.x, (int)inputTaskBox.y, (int)inputTaskBox.width, (int)inputTaskBox.height, isInputBoxActive ? COLOR_ACCENT : LIGHTGRAY);

    DrawText(inputTaskText, (int)inputTaskBox.x + 10, (int)inputTaskBox.y + 10, 20, COLOR_TEXT);
    if (isInputBoxActive && (letterCount < MAX_INPUT_CHARS) && ((GetTime() / 0.5) < (int)(GetTime() / 0.5))) {
        DrawText("_", (int)inputTaskBox.x + 10 + MeasureText(inputTaskText, 20), (int)inputTaskBox.y + 10, 20, COLOR_TEXT);
    }
    if (!isInputBoxActive && letterCount == 0) {
        DrawText("Add new activity...", (int)inputTaskBox.x + 10, (int)inputTaskBox.y + 10, 20, LIGHTGRAY);
    }

    // 2. Button ADD
    DrawRectangleRec(addButton, COLOR_ACCENT);
    DrawText("ADD", (int)addButton.x + 45, (int)addButton.y + 10, 20, COLOR_TASK_BG);

    int taskY      = 200;
    int taskHeight = 50;
    int spacing    = 10;

    for (const auto& task : tasks) {
        Rectangle taskRec         = { 50, (float)taskY, (float)screenWidth - 100, (float)taskHeight };
        Rectangle checkBoxRec     = { taskRec.x + 10, taskRec.y + 15, 20, 20 };
        Rectangle deleteButtonRec = { taskRec.x + taskRec.width - 100, taskRec.y + 10, 90, 30 };

        DrawRectangleRounded(taskRec, 0.2f, 8, COLOR_TASK_BG);
        DrawRectangleRoundedLines(taskRec, 0.2f, 8, LIGHTGRAY);

        // Checkbox
        DrawRectangleRec(checkBoxRec, task.done ? COLOR_DONE : LIGHTGRAY);
        DrawRectangleLinesEx(checkBoxRec, 2, COLOR_ACCENT);
        if (task.done) {
            DrawText("X", (int)checkBoxRec.x + 4, (int)checkBoxRec.y + 1, 20, COLOR_ACCENT);
        }

        // Task text
        DrawText(task.description.c_str(), (int)taskRec.x + 40, (int)taskRec.y + 15, 20, COLOR_TEXT);
        if (task.done) {
            DrawLine((int)taskRec.x + 40, (int)taskRec.y + 25, (int)taskRec.x + 40 + MeasureText(task.description.c_str(), 20), (int)taskRec.y + 25, GRAY);
        }

        // DELETE button
        DrawRectangleRec(deleteButtonRec, COLOR_DELETE);
        DrawText("DELETE", (int)deleteButtonRec.x + 10, (int)deleteButtonRec.y + 7, 18, COLOR_TASK_BG);

        taskY += taskHeight + spacing;
    }

    if (tasks.empty()) {
         DrawText("No suspended activities. Add a new one!", 
            screenWidth/2 - MeasureText("No suspended activities. Add a new one!", 20)/2, 300, 20, LIGHTGRAY);
    }
    EndDrawing();
}

int main(void) {
    InitWindow(screenWidth, screenHeight, "Raylib To-Do App");
    SetTargetFPS(60);

    InitDatabase();
    LoadTasks();

    while (!WindowShouldClose()) {
        HandleInput();
        DrawUI();
    }

    sqlite3_close(db);
    CloseWindow(); 
    return 0;
}