#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <stack>
#include <functional>
#include <time.h>
#include "logger.h"

class Parser;

using PtrFcn = void(*)(const char**, Parser* const);

enum class TypeChar {
    Digit,
    Colon,
    Dot,
    Minus,
    Comma,
    CommaInQ,
    Equal,
    Quote,
    Dquote,
    NewLine,
    NewLineInQ,
    Char
};

class EventData {
    //ВНИМАНИЕ!!!
    //При добавлении новой переменной добавить в конструктор копирования и перемещения
    std::uint64_t offset_start_;
    std::uint64_t offset_end_;

    std::uint8_t minute_;
    std::uint8_t second_;
    std::uint32_t msecond_;
    std::size_t duration_;
    std::string name_;
    int level_;
    std::vector<std::pair<std::string, std::string>> values_;
    std::string key_temp_;
    std::string value_temp_;
    std::string empty_string_;

    // Массив для хранения наличия конкретного свойства
    // 0 - Context
    // 1 - Sql
    // 2 - DataBase
    // 3 - t:computerName
    // 4 - SessionID
    // 5 - WaitConnections
    // 6 - Usr
    // 7 - DeadlockConnectionIntersections
    std::array<bool, 8> is_key_index_;
    // Массив для хранения индекса конкретного свойства в values_
    // 0 - Context
    // 1 - Sql
    // 2 - DataBase
    // 3 - t:computerName
    // 4 - SessionID
    // 5 - WaitConnections
    // 6 - Usr
    // 7 - DeadlockConnectionIntersections
    std::array<std::uint64_t, 8> key_index_;   

    void EndValue();
    const std::string* GetProperty(std::uint64_t) const;

    friend void Start(const char** ch, Parser* const parser);
    friend void Minute(const char** ch, Parser* const parser);
    friend void Second(const char** ch, Parser* const parser);
    friend void Msecond(const char** ch, Parser* const parser);
    friend void Duration(const char** ch, Parser* const parser);
    friend void Event(const char** ch, Parser* const parser);
    friend void Level(const char** ch, Parser* const parser);
    friend void Key(const char** ch, Parser* const parser);
    friend void Value(const char** ch, Parser* const parser);
    friend void EndValue(const char** ch, Parser* const parser);
    friend void EndEvent(const char** ch, Parser* const parser);
    friend void Mvalue(const char** ch, Parser* const parser);

    //friend std::ostream& operator<< (std::ostream& out, const EventData& event_data);
public:
    EventData();
    EventData(EventData&& event_data) noexcept;
    EventData& operator=(EventData&& event_data) noexcept;

    std::uint64_t OffsetStart() { return offset_start_; }
    std::uint64_t OffsetEnd() { return offset_end_; }
    std::uint64_t Lenght() { return offset_end_ - offset_start_; }
    std::uint8_t Minute() const { return minute_; }
    std::uint8_t Second() const { return second_; }
    std::uint32_t Msecond() const { return msecond_; }
    std::size_t Duration() const { return duration_; }
    const std::string& Name() const { return name_; }
    int Level() const { return level_; }
    const std::string* GetContext() const { return GetProperty(0); }
    const std::string* GetSql() const { return GetProperty(1); }
    const std::string* GetDatabase() const { return GetProperty(2); }
    const std::string* GetComputer() const { return GetProperty(3); }
    const std::string* GetSession() const { return GetProperty(4); }
    const std::string* GetWaitConnections() const { return GetProperty(5); }
    const std::string* GetUsr() const { return GetProperty(6); }
    const std::string* GetDeadlockConnectionIntersections() const { return GetProperty(7); }
};

class Parser {
    std::vector<EventData> events_;
    std::stack<char> stack_quotes;
    EventData event_data;
    PtrFcn ptr_fcn;
    PtrFcn ptr_fcn_break;
    std::uint64_t offset_;

    friend void Start(const char** ch, Parser* const parser);
    friend void Minute(const char** ch, Parser* const parser);
    friend void Second(const char** ch, Parser* const parser);
    friend void Msecond(const char** ch, Parser* const parser);
    friend void Duration(const char** ch, Parser* const parser);
    friend void Event(const char** ch, Parser* const parser);
    friend void Level(const char** ch, Parser* const parser);
    friend void Key(const char** ch, Parser* const parser);
    friend void Value(const char** ch, Parser* const parser);
    friend void EndValue(const char** ch, Parser* const parser);
    friend void EndEvent(const char** ch, Parser* const parser);
    friend void Mvalue(const char** ch, Parser* const parser);

    friend TypeChar GetTypeChar(char ch, Parser* const parser);
public:
    Parser();
    void Parse(const char* ch, int size);
    void AddEvent(EventData&& event_data);
    std::vector<EventData>&& MoveEvents();
    void Clear();
};

