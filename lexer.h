#ifndef LEXER_H
#define LEXER_H

#include <string.h>

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
        DecimalQuotient, // dec or flt (no 0)
        NumberLeadingZero,
        FloatOrOctal, // had 0
        HexNumber,
        BinNumber,
        FloatQuotient,
        FloatFraction,
        FloatExponent,
        FloatSignedExponent,
    };

    enum Token
    {
        white_space,
        string_delimiter,
        string_escape,
        string_error, // \n inside string
        string_char,
        temp_slash, // can turn into operator (default) or comment
        number_char,
        error_char, // \ outside of string!
        operator_char,
        identifier,
        keyword,
        line_comment,
        block_comment,
        parenthesis,
    };

    State state;
    char buf[16];
    int len;

    int Get(char c) /*returns token | (back_num<<8)*/
    {
        static const char* match[] = 
        {
            "console", "ak",
            "await", "break", "case", "catch", "class",
            "const", "continue", "debugger", "default", "delete",
            "do", "else", "enum", "export", "extends",
            "false", "finally", "for", "function", "if",
            "implements", "import", "in", "instanceof", "interface",
            "let", "new", "null", "package", "private",
            "protected", "public", "return", "super", "switch",
            "static", "this", "throw", "try", "true",
            "typeof", "var", "void", "while", "with", 
            "yield", 0
        };

        switch (state)
        {
            case Pure:
            {
                switch (c)
                {
                    case ' ':
                    case '\n':
                        return white_space;

                    case '\"': 
                        state = StringDouble;
                        return string_delimiter;

                    case '\'': 
                        state = StringSingle;
                        return string_delimiter;

                    case '/':
                        state = SlashCommentOrDiv;
                        // could be operator / or comment // or comment /*
                        return temp_slash;

                    case '0':
                        state = NumberLeadingZero;
                        return number_char;

                    case '\\':
                        return error_char;

                    case '.':
                        state = FloatOrMember;
                        return operator_char; // may be float or member op !!!!

                    default:
                    if (c>='1' && c<='9')
                    {
                        state = DecimalQuotient;
                        return number_char;
                    }
                    else
                    if (strchr("!%^&*-+=:;,.?<>|~",c))
                    {
                        return operator_char;
                    }
                    else
                    if (strchr("{}()[]",c))
                    {
                        return parenthesis;
                    }
                    else
                    if (c>='a' && c<='z' || c>='A' && c<='Z' || c=='_' || c=='$')
                    {
                        state = Identifier;
                        len = 1;
                        buf[0] = c;
                        return identifier;
                    }
                    else
                    {
                        return error_char;
                    }
                }						
                break;
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

                    case 'x':
                    case 'X':
                    {
                        state = StringDoubleHex;
                        len = 0;
                        return string_escape;
                    }

                    default:
                    if (c>='0' && c<='3')
                    {
                        len = 1;
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
                    case 'X':
                    {
                        state = StringSingleHex;
                        len = 0;
                        return string_escape;
                    }

                    default:
                    if (c>='0' && c<='3')
                    {
                        len = 1;
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
                    len++;
                    if (len==3)
                        state = StringDouble;
                    return string_escape;
                }
                
                state = StringDouble;
                return string_char;
            }

            case StringSingleOct:
            {
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
                    len++;
                    if (len==3)
                        state = StringSingle;
                    return string_escape;
                }
                
                state = StringSingle;
                return string_char;
            }

            case StringDoubleHex:
            {
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
                    len++;
                    if (len==2)
                        state = StringDouble;
                    return string_escape;
                }
                
                state = StringDouble;
                return string_char;
            }

            case StringSingleHex:
            {
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
                    len++;
                    if (len==2)
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
                    if (len<15)
                    {
                        buf[len++]=c;
                        buf[len]=0;
                        
                        for (int k=0; match[k]; k++)
                        {
                            if (strcmp(buf,match[k])==0)
                            {
                                state = Keyword;
                                // recolor identifier into keyword
                                return keyword | ((len-1)<<8);
                            }
                        }
                    }
                    return identifier;
                }
                else
                {
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
                    if (len<15)
                    {
                        buf[len++]=c;
                        buf[len]=0;
                        
                        for (int k=0; match[k]; k++)
                        {
                            if (strcmp(buf,match[k])==0)
                                return keyword;
                        }
                    }
                    state = Identifier;

                    // recolor keyword into identifier
                    return identifier | ((len-1)<<8);
                }
                else
                {
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
                    state=FloatExponent;
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

            default:
                state = Pure;
                return Get(c);
        }

        state = Pure;
        return Get(c);
    }
};

#endif