// package com.github.jsonldjava.core;

// import java.util.regex.Pattern;


const Pattern TRICKY_UTF_CHARS = Pattern/*.compile*/ (
                                     // ("1.7".equals(System.getProperty("java.specification.version")) ?
                                     // "[\\x{10000}-\\x{EFFFF}]" :
                                     u"[\uD800\uDC00-\uDB7F\uDFFF]"s // this seems to work with jdk1.6
                                 );
// for ttl
const Pattern PN_CHARS_BASE = Pattern
                              /*.compile*/ ( "[a-zA-Z]|[\\u00C0-\\u00D6]|[\\u00D8-\\u00F6]|[\\u00F8-\\u02FF]|[\\u0370-\\u037D]|[\\u037F-\\u1FFF]|"s
                                      + "[\\u200C-\\u200D]|[\\u2070-\\u218F]|[\\u2C00-\\u2FEF]|[\\u3001-\\uD7FF]|[\\uF900-\\uFDCF]|[\\uFDF0-\\uFFFD]|"s
                                      + TRICKY_UTF_CHARS );
const Pattern PN_CHARS_U = Pattern/*.compile*/ ( PN_CHARS_BASE + "|[_]"s );
const Pattern PN_CHARS = Pattern/*.compile*/ ( PN_CHARS_U
                         + "|[-0-9]|[\\u00B7]|[\\u0300-\\u036F]|[\\u203F-\\u2040]"s );
const Pattern PN_PREFIX = Pattern/*.compile*/ ( "(?:(?:"s + PN_CHARS_BASE + ")(?:(?:"s
                          + PN_CHARS + "|[\\.])*(?:"s + PN_CHARS + "))?)"s );
const Pattern HEX = Pattern/*.compile*/ ( "[0-9A-Fa-f]"s );
const Pattern PN_LOCAL_ESC = Pattern
                             /*.compile*/ ( "[\\\\][_~\\.\\-!$&'\\(\\)*+,;=/?#@%]"s );
const Pattern PERCENT = Pattern/*.compile*/ ( "%"s + HEX + HEX );
const Pattern PLX = Pattern/*.compile*/ ( PERCENT + "|" + PN_LOCAL_ESC );
const Pattern PN_LOCAL = Pattern/*.compile*/ ( "((?:" + PN_CHARS_U + "|[:]|[0-9]|"
                         + PLX + ")(?:(?:"s + PN_CHARS + "|[.]|[:]|"s + PLX + ")*(?:"s + PN_CHARS + "|[:]|"s + PLX
                         + "))?)"s );
const Pattern PNAME_NS = Pattern/*.compile*/ ( "((?:"s + PN_PREFIX + ")?):"s );
const Pattern PNAME_LN = Pattern/*.compile*/ ( ""s + PNAME_NS + PN_LOCAL );
const Pattern UCHAR = Pattern/*.compile*/ ( "\\u005Cu"s + HEX + HEX + HEX + HEX
                      + "|\\u005CU"s + HEX + HEX + HEX + HEX + HEX + HEX + HEX + HEX );
const Pattern ECHAR = Pattern/*.compile*/ ( "\\u005C[tbnrf\\u005C\"']"s );
const Pattern IRIREF = Pattern/*.compile*/ ( "(?:<((?:[^\\x00-\\x20<>\"{}|\\^`\\\\]|"s
                       + UCHAR + ")*)>)"s );
const Pattern BLANK_NODE_LABEL = Pattern/*.compile*/ ( "(?:_:((?:"s + PN_CHARS_U
                                 + "|[0-9])(?:(?:"s + PN_CHARS + "|[\\.])*(?:"s + PN_CHARS + "))?))"s );
const Pattern WS = Pattern/*.compile*/ ( "[ \t\r\n]"s );
const Pattern WS_0_N = Pattern/*.compile*/ ( WS + "*"s );
const Pattern WS_0_1 = Pattern/*.compile*/ ( WS + "?"s );
const Pattern WS_1_N = Pattern/*.compile*/ ( WS + "+"s );
const Pattern STRING_LITERAL_QUOTE = Pattern
                                     /*.compile*/ ( "\"(?:[^\\u0022\\u005C\\u000A\\u000D]|(?:"s + ECHAR + ")|(?:"s + UCHAR + "))*\""s );
const Pattern STRING_LITERAL_SINGLE_QUOTE = Pattern
        /*.compile*/ ( "'(?:[^\\u0027\\u005C\\u000A\\u000D]|(?:"s + ECHAR + ")|(?:"s + UCHAR + "))*'"s );
const Pattern STRING_LITERAL_LONG_SINGLE_QUOTE = Pattern
        /*.compile*/ ( "'''(?:(?:(?:'|'')?[^'\\\\])|"s + ECHAR + "|"s + UCHAR + ")*'''"s );
const Pattern STRING_LITERAL_LONG_QUOTE = Pattern
        /*.compile*/ ( "\"\"\"(?:(?:(?:\"|\"\")?[^\\\"\\\\])|"s + ECHAR + "|"s + UCHAR + ")*\"\"\""s );
const Pattern LANGTAG = Pattern/*.compile*/ ( "(?:@([a-zA-Z]+(?:-[a-zA-Z0-9]+)*))"s );
const Pattern INTEGER = Pattern/*.compile*/ ( "[+-]?[0-9]+"s );
const Pattern DECIMAL = Pattern/*.compile*/ ( "[+-]?[0-9]*\\.[0-9]+"s );
const Pattern EXPONENT = Pattern/*.compile*/ ( "[eE][+-]?[0-9]+"s );
const Pattern DOUBLE = Pattern/*.compile*/ ( "[+-]?(?:(?:[0-9]+\\.[0-9]*"s + EXPONENT
                       + ")|(?:\\.[0-9]+"s + EXPONENT + ")|(?:[0-9]+"s + EXPONENT + "))"s );
