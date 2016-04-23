#include <cstdio>
#include <string>
#include <cctype>

#include <iostream>
#include <deque>
#include <map>
#include <string>

using namespace std;

bool need = true;

#include "astnode.h"
#include "parser.h"
#include "token.h"
#include "op.h"

ASTNode* expr();
Precedence* next_op();
ASTNode* do_shift(ASTNode* l, int prec);
ASTNode* statement();

#define OP_PRECEDENCE

void err(const char *msg, const std::string &str, bool end=true)
{
  if (str=="\n")
    printf("%s, token: eol\n", msg);
  else
    printf("%s, token: %s\n", msg, str.c_str());
  if (end)
    exit(1);
}

std::map<std::string, Precedence*> operators;

extern std::deque <Token> tokens;



Token peek_token()
{
  if (tokens.size() == 0)
    return invalid_token;
  else
    return tokens[0];
}

Token pop_token()
{
  if (tokens.size() == 0)
    return invalid_token;
  else
  {
    Token t = tokens.front();
    tokens.pop_front();
    return t;
  }
}

bool is_token(const std::string &str)
{
  Token t = peek_token();
  if (t.str_ == str)
    return true;
  else
    return false;
}



ASTNode* term()
{
  ASTNode *left = factor();
  while(is_token("*") || is_token("/"))
  {
    Token t = pop_token();
    ASTNode *op = new ASTNode(t);
    ASTNode *right = factor();
    op->add_child(left, right);
    left = op;
  }

  return left;
}

#ifdef OP_PRECEDENCE

/* p 45 EBNF
 * primary   : "(" expr ")" | NUMBER | IDENTIFIER | STRING
 * factor    : "-" primary | primary
 * expr      : factor { OP factor}
 * block     : "{" [ statement ] { ("; " | EOL) [ statement ]} "}"
 * simple    : expr
 * statement :   "if" expr block [ "else" block ] 
 *             | "while" expr block
 *             | simple
 * program   : [ statement ] ("; " | EOL)
 */

// primary   : "(" expr ")" | NUMBER | IDENTIFIER | STRING
ASTNode* primary()
{
  Token token = peek_token(); 
  if (token.str_ == "(")
  {
    Token t = pop_token();
    ASTNode *e = expr();
    t = pop_token();
    if (t.str_ != ")")
    {
      cout << "fail: should ')'" << endl;
      exit(1);
    }
    return e;
  }
  else if (token.type_ == NUMBER || token.type_ == NAME) // number || variable name
       {
         Token t = pop_token();
         return new ASTNode(t);
       }
       else if (token.type_ == STRING)
            {
            }
            else
            {
              if (need == true)
                err("primary: no match primer rule", token.str_);
              else
                err("primary: warn!!", token.str_, false);
            }
  return 0;
}

// factor    : "-" primary | primary
ASTNode* factor()
{
  ASTNode *op = 0;

  Token token = peek_token(); 
  if (token.str_ == "-")
  {
    Token t = pop_token();
    op = new ASTNode(t);
    t.type_ = NEG;
    ASTNode *p = primary();
    op->add_child(p);
  }
  else
  {
    op = primary();
  }

  return op;
}

// expr      : factor { OP factor} {eol}
ASTNode* expr()
{
  ASTNode *r = factor();
  Precedence *next;

  while((next = next_op()) != 0)
  {
    r = do_shift(r, next->value_);
    // left = new ASTNode(left, , right);
  }
  return r;
}

// block     : "{" [ statement ] { ("; " | EOL) [ statement ] } "}"
ASTNode* block()
{
  Token token = peek_token(); 
  ASTNode *b = 0;

  if (token.str_ == "{")
  {
    pop_token();

    need = false;
    b = statement();
    need = true;
    while(is_token(";") || is_token("\n"))
    {
      pop_token();

      need = false;
      ASTNode *s = statement();
      need = true;
      if (s==0)
        continue;
      if (b)
        b->add_child(s);
      else
        b=s; // s is possiable 0??
    }

    Token t = peek_token();
    if (t.str_ == "}")
    {
      Token t = pop_token();
    }
    else
    {
      err("block: should '}'", t.str_);
    }


  }
  else
  {
    err("block: should '{'", token.str_);
  }
  return b;
}

// simple    : expr
// modify: simple    : expr { eol }
ASTNode* simple()
{
  ASTNode *e=0;
  e = expr();
  #if 0
  while(is_token("\n"))
  {
    pop_token();
  }
  #endif
  return e;
}

/*
 * statement :   "if" expr block [ "else" block ] 
 *             | "while" expr block
 *             | simple
 */
ASTNode* statement()
{
  ASTNode *s_node = 0; // statement node
  Token token = peek_token(); 

  if (token.str_ == "if")
  {
    Token t = pop_token();
    t.type_ = IF;
    s_node = new ASTNode(t);
    ASTNode *e = expr();

    ASTNode *then_b = block();


    s_node->add_child(e, then_b);

    Token token = peek_token(); 
    if (token.str_ == "else")
    {
      Token t = pop_token();
      ASTNode *else_b = block();
      s_node->add_child(else_b);
    }
    #if 0
    else
    {
      err("statement: should 'else'", token.str_);
    }
    #endif

  }
  else if (token.str_ == "while")
       {
         Token t = pop_token();
         t.type_ = WHILE;
         s_node = new ASTNode(t);

         ASTNode *e = expr();
         ASTNode *b = block();
         s_node->add_child(e, b);
       }
       else // simple
       {
         s_node = simple();
       }

  return s_node;
}

// program   : [ statement ] ("; " | EOL)
ASTNode* program()
{
  ASTNode *l = 0;
  need = false;
  l = statement();
  need = true;
  Token token = peek_token(); 
#if 1
  if (token.str_ == ";" || token.str_ == "\n")
  {
    pop_token();
  }
  else
  {
    err("program: should ; or eol", token.str_);
  }
#endif
  return l;
}

// operator precedence parsing
/*
 *  factor: NUMBER | "(" expression ")"
 *  expression: factor { OP factor}
 */

Precedence* next_op()
{
  Token t = peek_token();
  auto it = operators.find(t.str_);

  if (it != operators.end())
    return it->second;
  else
    return 0;
}

bool right_is_expr(int prec, Precedence* next_prec)
{
  if (next_prec->left_assoc_)
    return prec < next_prec->value_;
  else
    return prec <= next_prec->value_;
}

ASTNode* do_shift(ASTNode* l, int prec)
{
  Token t = pop_token();
  ASTNode *op = new ASTNode(t);
  ASTNode *r = factor();
  Precedence *next;

  while((next = next_op()) != 0 && right_is_expr(prec, next))
  {
    r = do_shift(r, next->value_);
  }

  op->add_child(l, r);
  return op;
}

ASTNode* expression()
{
  ASTNode *r = factor();
  Precedence *next;

  while((next = next_op()) != 0)
  {
    r = do_shift(r, next->value_);
    // left = new ASTNode(left, , right);
  }
  #if 0
  #endif
  return r;
}
#else
/*
 *  factor: NUMBER | "(" expression ")"
 *  term: factor { ("*" | "/") factor}
 *  expression: term {("+" | "-") term}
 */

ASTNode* expression()
{
  ASTNode *left = term();
  while(is_token("+") || is_token("-"))
  {
    Token t = pop_token();
    ASTNode *op = new ASTNode(t);
    ASTNode *right = factor();
    op->add_child(left, right);
    left = op;
    // left = new ASTNode(left, , right);
  }

  return left;
}

#endif


#ifdef DEBUG_PARSER
int main(int argc, char *argv[])
{
  operators.insert({"=", new Precedence{1, false}});
  operators.insert({"==", new Precedence{2, true}});
  operators.insert({">", new Precedence{2, false}});
  operators.insert({"+", new Precedence{3, true}});
  operators.insert({"*", new Precedence{4, true}});

  int lexer();
  lexer(); 

  ASTNode* root=0;
  while (tokens.size()>0 && (root = program()))
  {
    cout << "ast node type: " << root->type_str() << endl;
    root->print();
    cout << endl;
  }
#if 0
  while(1)
  {
    string token;

    //int ret = get_token(token);
    int ret = get_se_token(token);
    if (ret == EOF)
    {
      break;
    }
    if (ret == OK)
    {
      cout << "token: " << token << endl;
    }
    else
    {
      cout << "token error" << endl;
    }
    token.clear();
  }
#endif
  return 0;
}
#endif
