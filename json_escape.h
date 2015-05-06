// http://stackoverflow.com/a/7725289

std::stringstream& jse(const std::string& input) {
    std::stringstream ss;
    for (auto iter = input.cbegin(); iter != input.cend(); iter++) {
        switch (*iter) {
            case '\\': ss << "\\\\"; break;
            case '"': ss << "\\\""; break;
            //case '/': ss << "\\/"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << *iter; break;
        }
    }
    return ss;
}

stringstream& jsq(stringstream &ss, const std::string& input) {
    ss << "\"" << jse(input) << "\"";
    return ss;
}