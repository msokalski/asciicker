#ifndef LEXER_H
#define LEXER_H

#include <assert.h>
#include <string.h>
#include <algorithm>

struct Lexer
{
    enum State
    {
        Pure,
        StringDouble,
        StringDoubleEsc,
        StringDoubleOct, 
        StringDoubleHex,
        StringSingle,
        StringSingleEsc,
        StringSingleOct,
        StringSingleHex,
        SlashCommentOrDiv,
        BlockComment,
        LineComment,
        BlockCommentAlmostEnd,
        Identifier,
        Keyword,
        FloatOrMember,
        DecimalQuotient,
        NumberLeadingZero,
        FloatOrOctal,
        HexNumber,
        BinNumber,
        FloatQuotient,
        FloatFraction,
        FloatExponent,
        FloatSignedExponent,

        StringTemplate,
        StringTemplateEsc,
        StringTemplateOct, 
        StringTemplateHex, 
        StringTemplateDol, // $ awaits {

        // these 3 states await '}' after any number of hex digits
        StringTemplateHexGrp, 
        StringDoubleHexGrp, 
        StringSingleHexGrp, 
    };

    enum Token
    {
        white_space,
        string_delimiter,
        string_escape,
        string_error,
        string_char,
        number_char,
        error_char,
        operator_char,
        identifier,
        keyword,
        line_comment,
        block_comment,
        bracket_rnd,
        bracket_sqr,
        bracket_crl,
        template_delimiter, // '${' and '}' but interior is colorized as reglar code!
    };

    uint8_t  state;  // main state 8 bits  
    uint8_t  depth;  // string termplate expression recursion depth (0-255)
    uint16_t idxlen; // shared between: keyword matcher state and escape oct/hex/hexgrp length
    uint32_t call;   // optional, shifted<<8 length of identifier call including whitespaces before '('

    static const int idx_bits = 9; // 512 keywords (adjustable)
    static const int len_bits = 15-idx_bits;
    static const int max_len  = (1<<len_bits)-1;
    static const int max_idx  = (1<<idx_bits)-1;

    uint32_t Get(char c) /*returns token | (back_num<<8)*/
    {
        struct Matcher
        {
            static uint16_t find(uint16_t state, char c)
            {
                // don't attack me, i already said there's no match!
                assert(state != 0xffff);
                static const char* match[] = 
                {
                    /*"await",*/ "break", "case", "catch", "class",
                    "const", "continue", "default", "delete",
                    "do", "else", "enum", "export", "extends",
                    "false", "finally", "for", "function", "if",
                    "implements", "import", "in", "instanceof", "interface",
                    "let", "new", "null", "package", "private",
                    "protected", "public", "return", "super", "switch",
                    "static", "this", "throw", "try", "true",
                    "typeof", "var", "void", "while", "with", /*"yield",*/

                    "ak", "akPrint", // these should have another color
                    "Object","Function","Array","Number","Boolean",
                    "String","Symbol","Date","Promise","RegExp","ArrayBuffer",
                    "Uint8Array","Int8Array","Uint16Array","Int16Array",
                    "Uint32Array","Int32Array","Float32Array","Float64Array",
                    "Uint8ClampedArray","BigUint64Array","BigInt64Array",
                    "DataView","Map","BigInt","Set","WeakMap","WeakSet",
                    "Proxy","Reflect","FinalizationRegistry","WeakRef",
                    "Error","AggregateError","EvalError","RangeError",
                    "ReferenceError","SyntaxError","TypeError","URIError",
                    "JSON","Math","Intl","decodeURI","decodeURIComponent",
                    "encodeURI","encodeURIComponent","escape","unescape",
                    "eval","isFinite","isNaN","parseFloat","parseInt",
                    "Infinity","NaN","undefined","globalThis"

                    //#endif
                };

                static bool init = true;
                const size_t size = sizeof(match)/sizeof(match[0]);
                /*statc_*/assert(size <= max_idx+1);
                static uint16_t index[256];

                if (init)
                {
                    init = false;
                    std::sort(match,match+size,[](const char* a, const char* b){ return strcmp(a,b)<0; });
                    memset(index,0xFF,size*sizeof(uint16_t));
                    for (uint16_t i=0; i<(uint16_t)size; i++)
                    {
                        assert(strlen(match[i])<=max_len);
                        if (index[match[i][0]] == 0xFFFF)
                            index[match[i][0]] = i;
                    }
                }

                if (!state)
                {
                    // coarse lookup
                    state = index[c];
                    if (state >= size)
                        return 0xFFFF;
                }

                int idx = state & max_idx;
                int len = (state >> idx_bits) & max_len;
                const char* org = match[idx];

                do
                {
                    if (match[idx][len] == c)
                    {
                        len++;
                        // exact or partial?
                        return idx | (len<<idx_bits) | (match[idx][len] ? 1<<15 : 0);
                    }

                    if (match[idx][len] > c)
                    {
                        // early rejection, 
                        // thanks to alphabetical order
                        break;
                    }

                    idx++;
                } while (idx < size && (!len || !strncmp(org,match[idx],len)));

                // no match
                return 0xFFFF;
            }
        };

        switch (state)
        {
            case Pure:
            {
                switch (c)
                {
                    case '[':
                    {
                        // call is already shifted!
                        uint32_t ret = bracket_sqr | call;
                        call=0;
                        return ret;
                    }

                    case ']':
                    {
                        call=0;
                        return bracket_sqr;
                    }

                    case '(':
                    {
                        // call is already shifted!
                        uint32_t ret = bracket_rnd | call;
                        call=0;
                        return ret;
                    }

                    case ')':
                    {
                        call=0;
                        return bracket_rnd;
                    }

                    case '{':
                    {
                        call=0;
                        if (depth)
                            return error_char;
                        return bracket_crl;
                    }

                    case '}':
                    {
                        call=0;
                        if (depth)
                        {
                            depth--;
                            state = StringTemplate;
                            return template_delimiter;
                        }
                        return bracket_crl;
                    }

                    case '`':
                        call=0;
                        state = StringTemplate;
                        return string_delimiter;

                    case '\"': 
                        call=0;
                        state = StringDouble;
                        return string_delimiter;

                    case '\'': 
                        call=0;
                        state = StringSingle;
                        return string_delimiter;

                    case '/':
                        call=0;
                        state = SlashCommentOrDiv;
                        // could be operator / or comment // or comment /*
                        return operator_char;

                    case '0':
                        call=0;
                        state = NumberLeadingZero;
                        return number_char;

                    case '\\':
                        call=0;
                        return error_char;

                    case '.':
                        call=0;
                        state = FloatOrMember;
                        return operator_char; // may be float or member op !!!!

                    default:
                    if (c>='1' && c<='9')
                    {
                        call=0;
                        state = DecimalQuotient;
                        return number_char;
                    }
                    else
                    if (strchr("!%^&*-+=:;,.?<>|~",c))
                    {
                        call=0;
                        return operator_char;
                    }
                    else
                    if (c>='a' && c<='z' || c>='A' && c<='Z' || c=='_' || c=='$')
                    {
                        call=0x100;
                        idxlen = Matcher::find(0,c);
                        if (idxlen>>15)
                        {
                            // unmatched 0xffff or partially matched 1<<15 | ...
                            state = Identifier;
                            return identifier;
                        }
                        else
                        {
                            // exact match
                            state = Keyword;
                            return keyword;
                        }
                    }
                    else
                    {
                        if (strchr(" \n\r\v\f\t",c))
                        {
                            if (call)
                                call+=0x100;
                            return white_space;
                        }

                        // probably \ or # or @ 
                        // or a special char 0-31 or anything above 126
                        call=0;
                        return error_char;
                    }
                }						
                break;
            }

            case StringTemplate:
            {
                switch (c)
                {
                    case '`': 
                        state = Pure;
                        return string_delimiter;
                    case '\\':
                        state = StringTemplateEsc;
                        return string_escape;
                    case '$':
                        state = StringTemplateDol;
                        return template_delimiter;
                    default:
                        return string_char;
                }
            }

            case StringTemplateDol:
            {
                if (c=='{')
                {
                    depth++;
                    state = Pure;
                    return template_delimiter;
                }

                // recolor $ back
                state = StringTemplate;
                return string_char | (1<<8);
            }

            case StringTemplateEsc:
            {
                switch (c)
                {
                    case 'u':
                    {
                        state = StringTemplateHex;
                        idxlen = 0;
                        return string_escape;
                    }

                    case 'x':
                    {
                        state = StringTemplateHex;
                        idxlen = 2;
                        return string_escape;
                    }

                    default:
                    if (c>='0' && c<='3')
                    {
                        idxlen = 1;
                        state = StringTemplateOct;
                        return string_escape;
                    }
                    state = StringTemplate;
                    return string_escape;
                }
            }

            case StringTemplateOct:
            {
                if (c=='\\')
                {
                    state = StringTemplateEsc;
                    return string_escape;
                }
                else
                if (c=='`')
                {
                    state = Pure;
                    return string_delimiter;
                }
                else
                if (c>='0' && c<='7')
                {
                    idxlen++;
                    if (idxlen==3)
                        state = StringTemplate;
                    return string_escape;
                }
                
                state = StringTemplate;
                return string_char;
            }

            case StringTemplateHex:
            {
                if (idxlen==0 && c=='{')
                {
                    state = StringTemplateHexGrp;
                    return string_escape;
                }

                if (c=='\\')
                {
                    state = StringTemplateEsc;
                    return string_escape;
                }
                else
                if (c=='`')
                {
                    state = Pure;
                    return string_delimiter;
                }
                else
                if (c>='0' && c<='9' || 
                    c>='a' && c<='f' ||
                    c>='A' && c<='F')
                {
                    idxlen++;
                    if (idxlen==4)
                        state = StringTemplate;
                    return string_escape;
                }
                
                state = StringTemplate;
                return string_char;
            }

            case StringDouble:
            {
                switch (c)
                {
                    case '\"': 
                        state = Pure;
                        return string_delimiter;
                    case '\\':
                        state = StringDoubleEsc;
                        return string_escape;
                    case '\n':
                        state = Pure;
                        return string_error;
                    default:
                        return string_char;
                }
                break;
            }

            case StringSingle:
            {
                switch (c)
                {
                    case '\'': 
                        state = Pure;
                        return string_delimiter;
                    case '\\':
                        state = StringSingleEsc;
                        return string_escape;
                    case '\n':
                        state = Pure;
                        return string_error;
                    default:
                        return string_char;
                }
                break;
            }

            case StringDoubleEsc:
            {
                switch (c)
                {
                    case '\n':
                    {
                        state = Pure;
                        return string_error;							
                    }

                    case 'u':
                    {
                        state = StringDoubleHex;
                        idxlen = 0;
                        return string_escape;
                    }

                    case 'x':
                    {
                        state = StringDoubleHex;
                        idxlen = 2;
                        return string_escape;
                    }

                    default:
                    if (c>='0' && c<='3')
                    {
                        idxlen = 1;
                        state = StringDoubleOct;
                        return string_escape;
                    }
                    state = StringDouble;
                    return string_escape;
                }
                break;
            }

            case StringSingleEsc:
            {
                switch (c)
                {
                    case '\n':
                    {
                        state = Pure;
                        return string_error;							
                    }

                    case 'x':
                    {
                        state = StringSingleHex;
                        idxlen = 2;
                        return string_escape;
                    }

                    case 'u':
                    {
                        state = StringSingleHex;
                        idxlen = 0;
                        return string_escape;
                    }

                    default:
                    if (c>='0' && c<='3')
                    {
                        idxlen = 1;
                        state = StringSingleOct;
                        return string_escape;
                    }
                    state = StringSingle;
                    return string_escape;
                }
                break;
            }

            case StringDoubleOct:
            {
                if (c=='\\')
                {
                    state = StringDoubleEsc;
                    return string_escape;
                }
                else
                if (c=='\"')
                {
                    state = Pure;
                    return string_delimiter;
                }
                else
                if (c=='\n')
                {
                    state = Pure;
                    return string_error;
                }
                else
                if (c>='0' && c<='7')
                {
                    idxlen++;
                    if (idxlen==3)
                        state = StringDouble;
                    return string_escape;
                }
                
                state = StringDouble;
                return string_char;
            }

            case StringSingleOct:
            {
                if (c=='\\')
                {
                    state = StringSingleEsc;
                    return string_escape;
                }
                else
                if (c=='\'')
                {
                    state = Pure;
                    return string_delimiter;
                }
                else
                if (c=='\n')
                {
                    state = Pure;
                    return string_error;
                }
                else
                if (c>='0' && c<='7')
                {
                    idxlen++;
                    if (idxlen==3)
                        state = StringSingle;
                    return string_escape;
                }
                
                state = StringSingle;
                return string_char;
            }

            case StringDoubleHex:
            {
                if (idxlen==0 && c=='{')
                {
                    state = StringDoubleHexGrp;
                    return string_escape;
                }

                if (c=='\\')
                {
                    state = StringDoubleEsc;
                    return string_escape;
                }
                else
                if (c=='\"')
                {
                    state = Pure;
                    return string_delimiter;
                }
                else
                if (c=='\n')
                {
                    state = Pure;
                    return string_error;
                }
                else
                if (c>='0' && c<='9' || 
                    c>='a' && c<='f' ||
                    c>='A' && c<='F')
                {
                    idxlen++;
                    if (idxlen==4)
                        state = StringDouble;
                    return string_escape;
                }
                
                state = StringDouble;
                return string_char;
            }

            case StringSingleHex:
            {
                if (idxlen==0 && c=='{')
                {
                    state = StringSingleHexGrp;
                    return string_escape;
                }

                if (c=='\\')
                {
                    state = StringSingleEsc;
                    return string_escape;
                }
                else
                if (c=='\'')
                {
                    state = Pure;
                    return string_delimiter;
                }
                else
                if (c=='\n')
                {
                    state = Pure;
                    return string_error;
                }
                else
                if (c>='0' && c<='9' || 
                    c>='a' && c<='f' ||
                    c>='A' && c<='F')
                {
                    idxlen++;
                    if (idxlen==4)
                        state = StringSingle;
                    return string_escape;
                }
                
                state = StringSingle;
                return string_char;	
            }

            case SlashCommentOrDiv:
            {
                switch (c)
                {
                    case '*':
                        state = BlockComment;
                        return block_comment | (1<<8); // recolor prev char as well!
                    case '/':
                        state = LineComment;
                        return line_comment | (1<<8); // recolor prev char as well!
                    default:
                        // rescan with state 0
                        state = Pure;
                        return Get(c);
                }
                break;
            }
            
            case BlockComment:
            {
                switch (c)
                {
                    case '*':
                        state = BlockCommentAlmostEnd;
                        return block_comment;
                    default:
                        return block_comment;
                }
                break;
            }

            case LineComment:
            {
                switch (c)
                {
                    case '\n':
                        state = Pure;
                        return block_comment;
                    default:
                        return block_comment;
                }
                break;
            }

            case BlockCommentAlmostEnd:
            {
                switch (c)
                {
                    case '*':
                        return block_comment;
                    case '/':
                        state = Pure;
                        return block_comment;
                    default:
                        state = BlockComment;
                        return block_comment;
                }
                break;
            }

            case Identifier:
            {
                if (c>='a' && c<='z' || c>='A' && c<='Z' ||
                    c=='_' || c=='$' || c>='0' && c<='9')
                {
                    if (call)
                        call+=0x100;
                    if (idxlen != 0xffff)
                    {
                        int len = (idxlen >> idx_bits) & max_len;
                        idxlen = Matcher::find(idxlen, c);
                        if (idxlen>>15)
                            return identifier;

                        state = Keyword;
                        return keyword | (len<<8);
                    }
                    return identifier;
                }
                else
                {
                    // do not clear it, 
                    // it can be white-space or actual call '('
                    // Pure state will handle it
                    // call=0;

                    // rescan
                    state = Pure;
                    return Get(c);
                }
                break;
            }

            case Keyword:
            {
                if (c>='a' && c<='z' || c>='A' && c<='Z' ||
                    c=='_' || c=='$' || c>='0' && c<='9')
                {
                    if (call)
                        call+=0x100;
                    int len = (idxlen >> idx_bits) & max_len;
                    idxlen = Matcher::find(idxlen, c);
                    if (idxlen >> 15)
                    {
                        state = Identifier;
                        return identifier | (len<<8);
                    }
                    return keyword;
                }
                else
                {
                    // clear it! we don't want while() if() for() etc
                    // to look like a func call
                    call=0;

                    // rescan
                    state = Pure;
                    return Get(c);
                }
                break;
            }

            case FloatOrMember: // . (float literal without quotient or member operator)
            {
                if (c>='0' && c<='9')
                {
                    state = FloatFraction;
                    return number_char | (1<<8);
                }
                else
                {
                    // rescan
                    state = Pure;
                    return Get(c);
                }

                break;
            }

            case DecimalQuotient: // 765 (decimal or float quotient)
            {
                if (c>='0' && c<='9')
                    return number_char;
                if (c=='e' || c=='E')
                {
                    state = FloatExponent;
                    return number_char;
                }
                if (c=='.')
                {
                    state = FloatFraction;
                    return number_char;
                }
                state = Pure;
                return Get(c);
            }

            case NumberLeadingZero: // 0 (number leading)
            {
                if (c>='0' && c<='7')
                {
                    state = FloatOrOctal;
                    return number_char;
                }
                else
                if (c>='8' && c<='9')
                {
                    state = DecimalQuotient;
                    return number_char;
                }
                else
                if (c=='x' || c=='X')
                {
                    state = HexNumber;
                    return number_char;
                }
                else
                if (c=='b' || c=='B')
                {
                    state = BinNumber;
                    return number_char;
                }
                else
                if (c=='e' || c=='E')
                {
                    state = FloatExponent;
                    return number_char;
                }
                else
                if (c=='.')
                {
                    state = FloatFraction;
                    return number_char;
                }
                state = Pure;
                return Get(c);
            }
            
            case FloatOrOctal: // 234
            {
                if (c>='0' && c<='7')
                {
                    return number_char;
                }					
                if (c>='8' && c<='9')
                {
                    state = FloatQuotient;
                    return number_char;
                }
                if (c=='e' || c=='E')
                {
                    state = FloatExponent;
                    return number_char;
                }
                if (c=='.')
                {
                    state = FloatFraction;
                    return number_char;
                }
                state = Pure;
                return Get(c);
            }

            case HexNumber: // 0x (hex integer)
            {
                if (c>='0' && c<='9' || c>='a' && c<='f' || c>='A' && c<='F')
                    return number_char;
                state = Pure;
                return Get(c);
            }

            case BinNumber: // 0b (bin integer)
            {
                if (c=='0' || c=='1')
                    return number_char;
                state = Pure;
                return Get(c);
            }

            case FloatQuotient:
            {
                if (c>='0' && c<='9')
                    return number_char;
                else
                if (c=='e' || c=='E')
                {
                    state = FloatExponent;
                    return number_char;
                }
                else
                if (c=='.')
                {
                    state = FloatFraction;
                    return number_char;
                }
                state = Pure;
                return Get(c);
            }

            case FloatFraction: // .42 fraction
            {
                if (c>='0' && c<='9')
                    return number_char;
                if (c=='e' || c=='E')
                {
                    state = FloatExponent;
                    return number_char;
                }
                state = Pure;
                return Get(c);
            }

            case FloatExponent: // exponent, awaits sign
            {
                if (c>='0' && c<='9' || c=='-' || c=='+')
                {
                    state = FloatSignedExponent;
                    return number_char;
                }
                state = Pure;
                return Get(c);
            }

            case FloatSignedExponent: // exponent sign, await decimal digits
            {
                if (c>='0' && c<='9')
                    return number_char;
                state = Pure;
                return Get(c);
            }

            case StringTemplateHexGrp:
            {
                switch (c)
                {
                    case '$':
                    {
                        state = StringTemplateDol;
                        return string_escape;
                    }

                    case '}':
                    {
                        state = StringTemplate;
                        return string_escape;
                    }

                    case '\\':
                    {
                        state = StringTemplateEsc;
                        return string_escape;
                    }

                    case '`':
                    {
                        state = Pure;
                        return string_delimiter;
                    }

                    default:
                    //if (idxlen<5)
                    {
                        if (c>='0' && c<='9' || 
                            c>='a' && c<='f' ||
                            c>='A' && c<='F')
                        {
                            if (idxlen<0xffff)
                                idxlen++;
                            return string_escape;
                        }
                    }
                }
                
                state = StringTemplate;
                return string_char;
            }

            case StringDoubleHexGrp:
            {
                switch (c)
                {
                    case '}':
                    {
                        state = StringDouble;
                        return string_escape;
                    }

                    case '\\':
                    {
                        state = StringDoubleEsc;
                        return string_escape;
                    }

                    case '\"':
                    {
                        state = Pure;
                        return string_delimiter;
                    }

                    case '\n':
                    {
                        state = Pure;
                        return string_error;
                    }

                    default:
                    // if (idxlen<5)
                    {
                        if (c>='0' && c<='9' || 
                            c>='a' && c<='f' ||
                            c>='A' && c<='F')
                        {
                            if (idxlen<0xffff)
                                idxlen++;
                            return string_escape;
                        }
                    }
                }
                
                state = StringDouble;
                return string_char;
            }

            case StringSingleHexGrp:
            {
                switch (c)
                {
                    case '}':
                    {
                        state = StringSingle;
                        return string_escape;
                    }

                    case '\\':
                    {
                        state = StringSingleEsc;
                        return string_escape;
                    }

                    case '\'':
                    {
                        state = Pure;
                        return string_delimiter;
                    }

                    case '\n':
                    {
                        state = Pure;
                        return string_error;
                    }

                    default:
                    //if (idxlen<5)
                    {
                        if (c>='0' && c<='9' || 
                            c>='a' && c<='f' ||
                            c>='A' && c<='F')
                        {
                            if (idxlen<0xffff)
                                idxlen++;
                            return string_escape;
                        }
                    }
                }
                
                state = StringSingle;
                return string_char;
            }

            default:
                state = Pure;
                return Get(c);
        }

        state = Pure;
        return Get(c);
    }
};

#endif