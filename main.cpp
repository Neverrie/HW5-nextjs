#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <algorithm>
#include <sqlite3.h>
#include <string>
#include "httplib.h" 

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

std::mutex dbMutex;

std::mutex logMutex;
bool running = true;


void cleanOldData() {
    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open("weather.db", &db);
    if (rc) {
        std::cerr << "Ошибка открытия базы данных: " << sqlite3_errmsg(db) << std::endl;
        return;
    }


    const char *sql1 = "DELETE FROM temperatures WHERE created_at < datetime('now', '-24 hours');";
    rc = sqlite3_exec(db, sql1, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка выполнения SQL-запроса (temperatures): " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Очистка temperatures выполнена успешно." << std::endl;
    }


    const char *sql2 = "DELETE FROM hourly_averages WHERE created_at < datetime('now', '-30 days');";
    rc = sqlite3_exec(db, sql2, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка выполнения SQL-запроса (hourly_averages): " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Очистка hourly_averages выполнена успешно." << std::endl;
    }


    const char *sql3 = "DELETE FROM daily_averages WHERE created_at < datetime('now', '-365 days');";
    rc = sqlite3_exec(db, sql3, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка выполнения SQL-запроса (daily_averages): " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Очистка daily_averages выполнена успешно." << std::endl;
    }

    sqlite3_close(db);
}

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void logTemperature(float temperature) {
    std::lock_guard<std::mutex> lock(dbMutex);

    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open("weather.db", &db);
    if (rc) {
        std::cerr << "Ошибка открытия базы данных: " << sqlite3_errmsg(db) << std::endl;
        return;
    }


    std::string sql = "INSERT INTO temperatures (temperature, created_at) VALUES (" + std::to_string(temperature) + ", datetime('now', 'localtime'));";
    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка выполнения SQL-запроса: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    sqlite3_close(db);
}


void calculateHourlyAverage(const std::vector<float>& temperatures) {
    float sum = 0;
    for (float temp : temperatures) {
        sum += temp;
    }
    float average = sum / temperatures.size();

    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open("weather.db", &db);
    if (rc) {
        std::cerr << "Ошибка открытия базы данных: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    std::string sql = "INSERT INTO hourly_averages (average, created_at) VALUES (" + std::to_string(average) + ", datetime('now', 'localtime'));";
    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка выполнения SQL-запроса: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    sqlite3_close(db);
}

void calculateDailyAverage(const std::vector<float>& temperatures) {
    float sum = 0;
    for (float temp : temperatures) {
        sum += temp;
    }
    float average = sum / temperatures.size();

    sqlite3 *db;
    char *errMsg = 0;
    int rc;

    rc = sqlite3_open("weather.db", &db);
    if (rc) {
        std::cerr << "Ошибка открытия базы данных: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    std::string sql = "INSERT INTO daily_averages (average, created_at) VALUES (" + std::to_string(average) + ", datetime('now', 'localtime'));";
    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Ошибка выполнения SQL-запроса: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    sqlite3_close(db);
}

void inputListener() {
    char input;
    while (running) {
        std::cin >> input;
        if (input == 'q') {
            running = false;
            std::cout << "Завершение работы программы..." << std::endl;
        }
    }
}

void readTemperature(const std::string& port) {
#ifdef _WIN32
    HANDLE hPipe = CreateFile(
        port.c_str(),
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка открытия канала на Windows! Код ошибки: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Канал успешно открыт: " << port << std::endl;
#else
    int fd = open(port.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Ошибка открытия порта на macOS/Linux!" << std::endl;
        return;
    }

    std::cout << "Порт успешно открыт: " << port << std::endl;
#endif

    std::vector<float> hourlyTemperatures;
    std::vector<float> dailyTemperatures;
    auto lastHour = std::chrono::system_clock::now();
    auto lastDay = std::chrono::system_clock::now();
    auto lastCleanup = std::chrono::system_clock::now();

    while (running) {
        char buffer[256];
#ifdef _WIN32
        DWORD bytesRead;
        bool success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);
        if (!success) {
            std::cerr << "Ошибка чтения из канала на Windows! Код ошибки: " << GetLastError() << std::endl;
            break;
        }
#else
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        if (bytesRead < 0) {
            std::cerr << "Ошибка чтения из порта на macOS/Linux!" << std::endl;
            break;
        }
#endif

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            float temperature = std::atof(buffer);
            logTemperature(temperature);

            hourlyTemperatures.push_back(temperature);
            dailyTemperatures.push_back(temperature);

            auto now = std::chrono::system_clock::now();


            if (std::chrono::duration_cast<std::chrono::hours>(now - lastHour).count() >= 1) {
                calculateHourlyAverage(hourlyTemperatures);
                hourlyTemperatures.clear();
                lastHour = now;
            }


            if (std::chrono::duration_cast<std::chrono::hours>(now - lastDay).count() >= 24) {
                calculateDailyAverage(dailyTemperatures);
                dailyTemperatures.clear();
                lastDay = now;
            }


            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCleanup).count() >= 60) {
                cleanOldData(); 
                lastCleanup = now;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

#ifdef _WIN32
    CloseHandle(hPipe);
#else
    close(fd);
#endif
}

void startHTTPServer() {
    httplib::Server svr;


    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"}, 
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"}, 
        {"Access-Control-Allow-Headers", "Content-Type"} 
    });


    svr.Get("/current_temperature", [](const httplib::Request &, httplib::Response &res) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    float currentTemp = 0;
    std::string timestamp;

    if (sqlite3_open("weather.db", &db) == SQLITE_OK) {
        const char *sql = "SELECT temperature, created_at FROM temperatures ORDER BY created_at DESC LIMIT 1;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                currentTemp = sqlite3_column_double(stmt, 0);
                timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(db);
    }


    res.set_content("Temperature: " + std::to_string(currentTemp) + ", Timestamp: " + timestamp, "text/plain");
}); 


    svr.Get("/hourly_averages", [](const httplib::Request &, httplib::Response &res) {
        sqlite3 *db;
        sqlite3_stmt *stmt;
        float hourlyAvg = 0;

        if (sqlite3_open("weather.db", &db) == SQLITE_OK) {
            const char *sql = "SELECT average FROM hourly_averages ORDER BY created_at DESC LIMIT 1;";
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    hourlyAvg = sqlite3_column_double(stmt, 0);
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_close(db);
        }

        res.set_content(std::to_string(hourlyAvg), "text/plain");
    });


    svr.Get("/daily_average", [](const httplib::Request &, httplib::Response &res) {
        sqlite3 *db;
        sqlite3_stmt *stmt;
        float dailyAvg = 0;

        if (sqlite3_open("weather.db", &db) == SQLITE_OK) {
            const char *sql = "SELECT average FROM daily_averages ORDER BY created_at DESC LIMIT 1;";
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    dailyAvg = sqlite3_column_double(stmt, 0);
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_close(db);
        }

        res.set_content(std::to_string(dailyAvg), "text/plain");
    });


    svr.Get("/last_temperatures", [](const httplib::Request &, httplib::Response &res) {
        sqlite3 *db;
        sqlite3_stmt *stmt;

        if (sqlite3_open("weather.db", &db) == SQLITE_OK) {
            const char *sql = "SELECT temperature FROM temperatures ORDER BY created_at DESC LIMIT 5;";
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
                std::string result;
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    float temperature = sqlite3_column_double(stmt, 0);
                    result += std::to_string(temperature) + ",";
                }
                sqlite3_finalize(stmt);
                res.set_content(result, "text/plain");
            } else {
                res.status = 500;
                res.set_content("Ошибка выполнения SQL-запроса", "text/plain");
            }
            sqlite3_close(db);
        } else {
            res.status = 500;
            res.set_content("Ошибка открытия базы данных", "text/plain");
        }
    });


svr.Get("/all_temperatures", [](const httplib::Request &, httplib::Response &res) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("weather.db", &db) == SQLITE_OK) {
        const char *sql = "SELECT temperature, created_at FROM temperatures ORDER BY created_at DESC;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            std::string result;
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                float temperature = sqlite3_column_double(stmt, 0);
                const char *timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                result += "Temperature: " + std::to_string(temperature) + ", Timestamp: " + std::string(timestamp) + "\n";
            }
            sqlite3_finalize(stmt);
            res.set_content(result, "text/plain");
        } else {
            res.status = 500;
            res.set_content("Ошибка выполнения SQL-запроса", "text/plain");
        }
        sqlite3_close(db);
    } else {
        res.status = 500;
        res.set_content("Ошибка открытия базы данных", "text/plain");
    }
});


svr.Get("/last_hourly_averages", [](const httplib::Request &, httplib::Response &res) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("weather.db", &db) == SQLITE_OK) {
        const char *sql = "SELECT average, created_at FROM hourly_averages ORDER BY created_at DESC;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            std::string result;
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                float average = sqlite3_column_double(stmt, 0);
                const char *timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                result += "Temperature: " + std::to_string(average) + ", Timestamp: " + std::string(timestamp) + "\n";
            }
            sqlite3_finalize(stmt);
            res.set_content(result, "text/plain");
        } else {
            res.status = 500;
            res.set_content("Ошибка выполнения SQL-запроса", "text/plain");
        }
        sqlite3_close(db);
    } else {
        res.status = 500;
        res.set_content("Ошибка открытия базы данных", "text/plain");
    }
});


svr.Get("/last_daily_averages", [](const httplib::Request &, httplib::Response &res) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("weather.db", &db) == SQLITE_OK) {
        const char *sql = "SELECT average, created_at FROM daily_averages ORDER BY created_at DESC;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            std::string result;
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                float average = sqlite3_column_double(stmt, 0);
                const char *timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                result += "Temperature: " + std::to_string(average) + ", Timestamp: " + std::string(timestamp) + "\n";
            }
            sqlite3_finalize(stmt);
            res.set_content(result, "text/plain");
        } else {
            res.status = 500;
            res.set_content("Ошибка выполнения SQL-запроса", "text/plain");
        }
        sqlite3_close(db);
    } else {
        res.status = 500;
        res.set_content("Ошибка открытия базы данных", "text/plain");
    }
});
svr.Get("/search_temperature", [](const httplib::Request &req, httplib::Response &res) {
    std::string datetime = req.get_param_value("datetime");
    std::cout << "Received search request for datetime: " << datetime << std::endl;
    sqlite3 *db;
    sqlite3_stmt *stmt;
    float temperature = 0;
    std::string timestamp;

    if (sqlite3_open("weather.db", &db) == SQLITE_OK) {
        const char *sql = "SELECT temperature, created_at FROM temperatures WHERE created_at = ?;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, datetime.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                temperature = sqlite3_column_double(stmt, 0);
                timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            }
            sqlite3_finalize(stmt);
        }
        sqlite3_close(db);
    }

    if (temperature != 0) {
        res.set_content("Temperature: " + std::to_string(temperature) + ", Timestamp: " + timestamp, "text/plain");
    } else {
        res.set_content("Not found", "text/plain");
    }
});

    std::cout << "HTTP-сервер запущен на http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
}



int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <порт>" << std::endl;
        return 1;
    }

    cleanOldData();
    std::string port = argv[1];


    std::thread httpThread(startHTTPServer);

    std::thread inputThread(inputListener);
    readTemperature(port);

    inputThread.join();
    httpThread.join();

    return 0;
}