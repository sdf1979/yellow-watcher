#include "parser.h"

static auto LOGGER = Logger::getInstance();

using namespace std;

EventData::EventData() :
    offset_start_(0),
    offset_end_(0),
    minute_(0),
    second_(0),
    msecond_(0),
    duration_(0),
    name_(""),
    level_(0),
    key_temp_(""),
    value_temp_(""),
    is_key_index_({ false, false, false, false, false, false, false, false, false, false, false, false, false, false }),
    key_index_({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })

{}

EventData::EventData(EventData&& event_data) noexcept :
    offset_start_(event_data.offset_start_),
    offset_end_(event_data.offset_end_),
    minute_(event_data.minute_),
    second_(event_data.second_),
    msecond_(event_data.msecond_),
    duration_(event_data.duration_),
    name_(move(event_data.name_)),
    level_(event_data.level_),
    key_temp_(move(event_data.key_temp_)),
    value_temp_(move(value_temp_)),
    values_(move(event_data.values_)),
    is_key_index_(event_data.is_key_index_),
    key_index_(event_data.key_index_)
{
    event_data.offset_start_ = 0;
    event_data.offset_end_ = 0;
    event_data.minute_ = 0;
    event_data.second_ = 0;
    event_data.msecond_ = 0;
    event_data.duration_ = 0;
    event_data.level_ = 0;
    event_data.is_key_index_ = { false, false, false, false, false, false, false, false, false, false, false, false, false, false };
    event_data.key_index_ = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
}

EventData& EventData::operator=(EventData&& event_data) noexcept {
    if (&event_data == this) return *this;

    offset_start_ = event_data.offset_start_;
    offset_end_ = event_data.offset_end_;
    minute_ = event_data.minute_;
    second_ = event_data.second_;
    msecond_ = event_data.msecond_;
    duration_ = event_data.duration_;
    name_ = move(event_data.name_);
    level_ = event_data.level_;
    key_temp_ = move(event_data.key_temp_);
    value_temp_ = move(value_temp_);
    values_ = move(event_data.values_);
    is_key_index_ = event_data.is_key_index_;
    key_index_ = event_data.key_index_;

    event_data.offset_start_ = 0;
    event_data.offset_end_ = 0;
    event_data.minute_ = 0;
    event_data.second_ = 0;
    event_data.msecond_ = 0;
    event_data.duration_ = 0;
    event_data.level_ = 0;
    event_data.is_key_index_ = { false, false, false, false, false, false, false, false, false, false, false, false, false, false };
    event_data.key_index_ = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        
    return *this;
}

const string* EventData::GetProperty(uint64_t index) const {
    if (is_key_index_[index]) {
        return &values_[key_index_[index]].second;
    }
    else {
        return &empty_string_;
    }
}

bool IsGuid(const char* begin, const char* end) {
    int size_guid = 0;
    for (; begin != end; ++begin) {
        char ch = *begin;
        if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch == '-' && (size_guid == 8 || size_guid == 13 || size_guid == 18 || size_guid == 23))) {
            ++size_guid;
        }
        else {
            return false;
        }
    }
    return true;
}

string ReplaceGuid(string value) {
    size_t size = value.size();
    if (size < 37) {
        return move(value);
    }
    else {
        const char* begin = value.c_str() + size - 36;
        if (IsGuid(begin, begin + 36)) {
            string new_value;
            new_value.append(value.substr(0, size - 36));
            new_value.append("_guid");
            return new_value;
        }
        else {
            return(move(value));
        }
    }
}

void EventData::EndValue() {
    if (key_temp_ == "Context") {
        is_key_index_[0] = true;
        key_index_[0] = values_.size();
        if (value_temp_[0] == '\'' && value_temp_[1] == '\r' && value_temp_[2] == '\n') {
            value_temp_.erase(1, 2);
        }
    }
    else if (key_temp_ == "Sql") {
        is_key_index_[1] = true;
        key_index_[1] = values_.size();
    }
    else if (key_temp_ == "DataBase") {
        is_key_index_[2] = true;
        key_index_[2] = values_.size();
    }
    else if (key_temp_ == "t:computerName") {
        is_key_index_[3] = true;
        key_index_[3] = values_.size();
    }
    else if (key_temp_ == "SessionID") {
        is_key_index_[4] = true;
        key_index_[4] = values_.size();
    }
    else if (key_temp_ == "WaitConnections") {
        is_key_index_[5] = true;
        key_index_[5] = values_.size();
    }
    else if (key_temp_ == "Usr") {
        is_key_index_[6] = true;
        key_index_[6] = values_.size();
    }
    else if (key_temp_ == "DeadlockConnectionIntersections") {
        is_key_index_[7] = true;
        key_index_[7] = values_.size();
    }
    else if (key_temp_ == "Rows") {
        is_key_index_[8] = true;
        key_index_[8] = values_.size();
    }
    else if (key_temp_ == "RowsAffected") {
        is_key_index_[9] = true;
        key_index_[9] = values_.size();
    }
    else if (key_temp_ == "planSQLText") {
        is_key_index_[10] = true;
        key_index_[10] = values_.size();
    }
    else if (key_temp_ == "Func") {
        is_key_index_[11] = true;
        key_index_[11] = values_.size();
    }
    else if (key_temp_ == "Regions") {
        is_key_index_[12] = true;
        key_index_[12] = values_.size();
    }
    else if (key_temp_ == "dbpid") {
        is_key_index_[13] = true;
        key_index_[13] = values_.size();
    }

    if (value_temp_.back() == '\r') {
        value_temp_.pop_back();
    }
    values_.push_back({ move(key_temp_), move(value_temp_) });
}

TypeChar GetTypeChar(char ch, Parser* const parser) {
    if (ch == -48 || ch == -47) {
        return TypeChar::Char;
    }
    else if (ch >= -128 && ch <= -65) {
        return TypeChar::Char;
    }
    else if (ch >= 'a' && ch <= 'z') {
        return TypeChar::Char;
    }
    else if (ch >= 'A' && ch <= 'Z') {
        return TypeChar::Char;
    }
    else if (ch >= '0' && ch <= '9') {
        return TypeChar::Digit;
    }
    else if (ch == ':') {
        return TypeChar::Colon;
    }
    else if (ch == '.') {
        return TypeChar::Dot;
    }
    else if (ch == '-') {
        return TypeChar::Minus;
    }
    else if (ch == ',') {
        if (parser->stack_quotes.empty()) {
            return TypeChar::Comma;
        }
        else {
            return TypeChar::CommaInQ;
        }
    }
    else if (ch == '=') {
        return TypeChar::Equal;
    }
    else if (ch == '\'') {
        return TypeChar::Quote;
    }
    else if (ch == '"') {
        return TypeChar::Dquote;
    }
    else if (ch == '\n') {
        if (parser->stack_quotes.empty()) {
            return TypeChar::NewLine;
        }
        else {
            return TypeChar::NewLineInQ;
        }
    }
    else {
        return TypeChar::Char;
    }
};

enum class State {
    Start,
    Minute,
    Second,
    Msecond,
    Duration,
    Event,
    Level,
    Key,
    Value,
    EndValue,
    EndEvent,
    Mvalue,
    Error
};

void Start(const char** ch, Parser* const parser) {
    parser->event_data = {};
    if (**ch == '\n') {
        ++* ch;
        ++parser->offset_;
    }
    parser->event_data.offset_start_ = parser->offset_;
}

void Minute(const char** ch, Parser* const parser) {
    parser->event_data.minute_ = parser->event_data.minute_ * 10 + (**ch - '0');
    ++* ch;
    ++parser->offset_;
}

void Second(const char** ch, Parser* const parser) {
    char ch_ = **ch;
    if (ch_ != ':') {
        parser->event_data.second_ = parser->event_data.second_ * 10 + (ch_ - '0');
    }
    ++* ch;
    ++parser->offset_;
}

void Msecond(const char** ch, Parser* const parser) {
    char ch_ = **ch;
    if (ch_ != '.') {
        parser->event_data.msecond_ = parser->event_data.msecond_ * 10 + (ch_ - '0');
    }
    ++* ch;
    ++parser->offset_;
}

void Duration(const char** ch, Parser* const parser) {
    char ch_ = **ch;
    if (ch_ != '-') {
        parser->event_data.duration_ = parser->event_data.duration_ * 10 + (ch_ - '0');
    }
    ++* ch;
    ++parser->offset_;
}

void Event(const char** ch, Parser* const parser) {
    char ch_ = **ch;
    if (ch_ != ',') {
        parser->event_data.name_.push_back(ch_);
    }
    ++* ch;
    ++parser->offset_;
}

void Level(const char** ch, Parser* const parser) {
    char ch_ = **ch;
    if (ch_ != ',') {
        parser->event_data.level_ = parser->event_data.level_ * 10 + (ch_ - '0');
    }
    ++* ch;
    ++parser->offset_;
}

void Key(const char** ch, Parser* const parser) {
    char ch_ = **ch;
    if (ch_ != ',') {
        parser->event_data.key_temp_.push_back(ch_);
    }
    ++* ch;
    ++parser->offset_;
}

void Value(const char** ch, Parser* const parser) {
    char ch_ = **ch;
    if (ch_ != '=' && ch_ != '\r') {
        parser->event_data.value_temp_.push_back(ch_);
    }
    ++* ch;
    ++parser->offset_;
}

void EndValue(const char** ch, Parser* const parser) {
    parser->event_data.EndValue();
}

void EndEvent(const char** ch, Parser* const parser) {
    parser->event_data.offset_end_ = parser->offset_;
    parser->events_.push_back(move(parser->event_data));
}

void Mvalue(const char** ch, Parser* const parser) {
    char ch_ = **ch;
    if (ch_ == '\'' || ch_ == '"') {
        if (parser->stack_quotes.empty()) {
            parser->stack_quotes.push(ch_);
        }
        else {
            if (parser->stack_quotes.top() == ch_) {
                parser->stack_quotes.pop();
            }
        }
    }
    parser->event_data.value_temp_.push_back(ch_);
    ++* ch;
    ++parser->offset_;
}

void Error(const char** ch, Parser* const parser) {
    LOGGER->Print("Parser error!", Logger::Type::Error);
    throw "Parser error!";
}

State GetState(PtrFcn ptr_fcn_) {
    if (ptr_fcn_ == Key) return State::Key;
    else if (ptr_fcn_ == Value) return State::Value;
    else if (ptr_fcn_ == Mvalue) return State::Mvalue;
    else if (ptr_fcn_ == EndValue) return State::EndValue;
    else if (ptr_fcn_ == Minute) return State::Minute;
    else if (ptr_fcn_ == Second) return State::Second;
    else if (ptr_fcn_ == Msecond) return State::Msecond;
    else if (ptr_fcn_ == Duration) return State::Duration;
    else if (ptr_fcn_ == Event) return State::Event;
    else if (ptr_fcn_ == Level) return State::Level;
    else if (ptr_fcn_ == EndEvent) return State::EndEvent;
    else if (ptr_fcn_ == Start) return State::Start;
    else return State::Error;
}

PtrFcn TransitionTable[12][12] = {
    //              Digit     Colon   Dot      Minus     Comma     CommaInQ Equal   Quote   Dquote  NewLine   NewLineInQ Char       
    /*Start*/     { Minute,   Error,  Error,   Error,    Error,    Error,   Error,  Error,  Error,  Error,    Error,     Error  },
    /*Minute*/    { Minute,   Second, Error,   Error,    Error,    Error,   Error,  Error,  Error,  Error,    Error,     Error  },
    /*Second*/    { Second,   Error,  Msecond, Error,    Error,    Error,   Error,  Error,  Error,  Error,    Error,     Error  },
    /*Msecond*/   { Msecond,  Error,  Error,   Duration, Error,    Error,   Error,  Error,  Error,  Error,    Error,     Error  },
    /*Duration*/  { Duration, Error,  Error,   Error,    Event,    Error,   Error,  Error,  Error,  Error,    Error,     Error  },
    /*Event*/     { Error,    Error,  Error,   Error,    Level,    Error,   Error,  Error,  Error,  Error,    Error,     Event  },
    /*Level*/     { Level,    Error,  Error,   Error,    Key,      Error,   Error,  Error,  Error,  Error,    Error,     Error  },
    /*Key*/       { Key,      Key,    Key,     Key,      Key,      Error,   Value,  Error,  Error,  Error,    Error,     Key    },
    /*Value*/     { Value,    Value,  Value,   Value,    EndValue, Error,   Value,  Mvalue, Mvalue, EndValue, Error,     Value  },
    /*EndValue*/  { Error,    Error,  Error,   Error,    Key,      Error,   Error,  Error,  Error,  EndEvent, Error,     Error  },
    /*EndEvent*/  { Error,    Error,  Error,   Error,    Error,    Error,   Error,  Error,  Error,  Start,    Error,     Error  },
    /*Mvalue*/    { Mvalue,   Mvalue, Mvalue,  Mvalue,   EndValue, Mvalue,  Mvalue, Mvalue, Mvalue, EndValue, Mvalue,    Mvalue }
};

Parser::Parser() :
    ptr_fcn(Start),
    ptr_fcn_break(nullptr),
    offset_(3) {}

void Parser::Parse(const char* ch, int size) {

    const char* end = ch + size - 1;

    //Обрабатываем первый символ
    if (ptr_fcn == Start) {
        ptr_fcn(&ch, this);
    }
    ptr_fcn = TransitionTable[(int)GetState(ptr_fcn)][(int)GetTypeChar(*ch, this)];

    //Обрабатываем от второго до предпоследнего
    for (; ch != end;) {
        ptr_fcn(&ch, this);
        ptr_fcn = TransitionTable[(int)GetState(ptr_fcn)][(int)GetTypeChar(*ch, this)];
    }

    //Обрабатываем крайний
    ++end;
    for (;;) {
        ptr_fcn(&ch, this);
        if (ch == end) {
            break;
        }
        ptr_fcn = TransitionTable[(int)GetState(ptr_fcn)][(int)GetTypeChar(*ch, this)];
    }
};

void Parser::AddEvent(EventData&& event_data) {
    events_.push_back(move(event_data));
}

std::vector<EventData>&& Parser::MoveEvents() {
    return move(events_);
}

void Parser::Clear() {
    events_.resize(0);
}
        
vector<string_view> split(string_view sv, const char* splitter) {
    vector<string_view> result;
    for (;;) {
        auto pos = sv.find(splitter);
        if (pos != string_view::npos) {
            result.push_back(sv.substr(0, pos));
            sv.remove_prefix(pos + 1);
        }
        else {
            result.push_back(sv);
            break;
        }
    }
    return result;
}

string_view ValueByKey(string_view sv, const string& key) {
    auto pos = sv.find(key);
    sv.remove_prefix(pos + key.size());
    string find = ",";
    if (sv.substr(0, 1) == "'") {
        find = "'";
        sv.remove_prefix(1);
    }
    pos = sv.find(find);
    if (pos != string_view::npos) {
        sv.remove_suffix(sv.size() - pos);
    }
    return sv;
}

