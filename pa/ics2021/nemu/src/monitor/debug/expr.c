#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
enum {
  TK_NOTYPE = 256,
  TK_EQ,
 	TK_HEX,
	TK_DEC,
/*	TK_EAX,
	TK_EBX,
	TK_ECX,
	TK_EDX,
	TK_EDI,
	TK_ESI,
	TK_EBP,
	TK_ESP,
	TK_EIP,*/
  TK_REG,
	TK_NQ,
	TK_AND,
	TK_OR,
	TK_MINUS,//负号
	TK_DEREF,//指针解引用
  /* TODO: Add more token types */
  TK_LE,
	TK_GE,
	TK_ML,
	TK_MR
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
    {" +", TK_NOTYPE},    // spaces
		{"==", TK_EQ},         // equal
		{"0x[0-9a-fA-F]{1,8}", TK_HEX}, //先检测16进制，再检测10进制，否则0x会被当做0 x
		{"[0-9]+", TK_DEC},
	/*	{"\\$eax", TK_EAX},
		{"\\$ebx", TK_EBX},
		{"\\$ecx", TK_ECX},
		{"\\$edx", TK_EDX},
		{"\\$edi", TK_EDI},
		{"\\$esi", TK_ESI},
		{"\\$ebp", TK_EBP},
		{"\\$esp", TK_ESP},
		{"\\$eip", TK_EIP},*/
		{"\\$[a-zA-Z]{2,3}", TK_REG},
		{"\\(", '('},
		{"\\)", ')'},
		{"\\+", '+'},
		{"-", '-'},	
		{"\\*", '*'},
		{"/", '/'},
		{"!=", TK_NQ},
		{"&&", TK_AND},
		{"\\|\\|", TK_OR},
		{"!", '!'},
	  {"~", '~'},
		{"%", '%'},
		{"\\|", '|'},
		{"&", '&'},
		{"\\^", '^'},
		{"<=", TK_LE},
		{">=", TK_GE},
		{"<<", TK_ML},
		{">>", TK_MR},
		{"<", '<'},
		{">", '>'}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;
	if(e == NULL) return false;
  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

       // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
       //     i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        if(substr_len >= 32){
					printf("%.*s  The length of the substring is too long.\n", substr_len, substr_start);								                //在用*%.\*s*时,后面跟着两个参数,一个表示输出数据占得位置的大小,一个表示要输出的内容
					return false;
				}

				if(nr_token >= 32) {
					printf("The count of tokens(nr_token) is out of the maximum count(32)\n");
					return false;
				}
				switch (rules[i].token_type) {
					case TK_NOTYPE: 
						break;
					case TK_DEC:
					case TK_HEX:
					case TK_REG:
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
					default:          
						tokens[nr_token].type = rules[i].token_type;													
						nr_token++;	  
						break;
				}
				break;
			}
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p, int q){
 	bool lr = false;
	if (tokens[p].type == '(' && tokens[q].type == ')'){
		lr = true;
	}

	int i, l = 0;

	for(i = p; i <= q; i++){// 先看括号是否匹配
		if(tokens[i].type == '(')  l++;
		else if(tokens[i].type == ')')	l--;
		if(l < 0){
			/*右括号先出现，如())*/
			printf("Bad Expression!\n");
			assert(0);            
		}
	}

	if(l != 0){  //左括号数量 > 右
		printf("Bad Expression!\n");
		assert(0);           
	}

	/*括号匹配，但是最外层没有()
	 4 + 3 * (2 - 1)	
 */

	if(!lr) return false;

	/*考虑这种情况
 	(4 + 3) * (2 - 1)
  */
	//此时 l == 0
	
	q-- , p++;
	for(i = p; i <= q; i++){
	 	if(tokens[i].type == '(')  l++;
		else if(tokens[i].type == ')')	l--;
		if(l < 0){
			return false;
		}
	}
	return true;
}

int op_priority(int op){ //获取运算符优先级
	int level;
		switch (op) {
			case TK_OR:// ||	
				level = 1;
				break;
			case TK_AND:// &&
				level = 2;
				break;
	 		case '|':
				level = 3;
				break;
			case '^':
				level = 4;
				break;
			case '&':
				level = 5;
				break;
			case TK_EQ:// ==  !=
			case TK_NQ:
				level = 6;
				break;
			case '>':
			case '<':
			case TK_LE:
			case TK_GE:
				level = 7;
				break;
			case TK_ML:
			case TK_MR:
				level = 8;
				break;  
			case '+':
			case '-':
				level = 9;
				break;
			case '*':
			case '/':
			case '%':
				level = 10;
				break;	
			case TK_DEREF:// 解引用
			case TK_MINUS:// 负号
			case '!':
			case '~':
				level = 11;//level == 11 一般都是单目运算符
				break;	
			default://不会出现，因为在使用正则表达式匹配时会扫描该运算符存不存在
				assert(0);
		}	
		return level;
}

int compare(int i, int pos){
		int priorityi = op_priority(tokens[i].type);
		int prioritypos = op_priority(tokens[pos].type);
		if(priorityi == prioritypos && prioritypos == 11) return 1;//如果是单目运算符，则最前面的优先级最大(即递归要从最前面的运算符开始)
		return priorityi - prioritypos; 
}

bool is_op(int ch){//是否为运算符
		return ch == '+' || ch == '-' || ch == '*' || ch == '/'	|| ch == '!' || ch == TK_AND || ch == TK_OR || ch == TK_EQ || ch == TK_NQ || ch == TK_DEREF || ch == TK_MINUS || ch == '|' || ch == '&' || ch == '^' || ch == '<' || ch == '>' || ch == TK_LE || ch == TK_GE || ch == TK_ML || ch == TK_MR || ch == '%' || ch == '~'	; 
}

int find_dominated_op(int p, int q){
		/*先判断是否为运算符
		 * 	如果是，判断它的等级，等级高于已经遍历过的运算符，则跳过；如果小于等于，则重新赋值；
		 * 	检测到（，则一直跳过，直到找到）  注意：不会存在括号不匹配的情况，如果存在程序已经终止 
		*/
		int pos = -1, i, opType, l = 0;//l 进行括号匹配 
		for(i = p; i <= q; i++){
			opType = tokens[i].type;		
			if(l == 0 && is_op(opType)){
				if(pos == -1 || compare(i, pos) <= 0) pos = i; //如果是第一个运算符，或者i的优先级小于等于pos的	
			}
			else if(opType == '(') l++;
			else if(opType == ')') l--;
		}
		return pos;
}

bool opEvalMinus(int op){
	    switch(op){
				case TK_OR:// ||
				case TK_AND:// &&
				case TK_EQ:// ==  !=
				case TK_NQ:
				case '+':
				case '-':
				case '*':
				case '/':
				case '(':
				case '!':
        case '|':
				case '&':
				case '^':
				case '<':
				case '>':
				case '%':
				case '~':
				case TK_LE:
				case TK_GE:
				case TK_ML:
				case TK_MR:
				case TK_MINUS:// 负号
				case TK_DEREF:       
					return true;				
				default:
					return false;
			}
}

bool opEvalDeref(int op){
	switch(op){
		case TK_OR:// ||
		case TK_AND:// &&
		case TK_EQ:// ==  !=
		case TK_NQ:
		case '+':
		case '-':
		case '*':	
		case '/':						
		case '(':
		case '!':
    case '|':
		case '&':
		case '^':
		case '<':
		case '>':
		case '%':
		case '~':
		case TK_LE:
		case TK_GE:
		case TK_ML:
		case TK_MR:
		case TK_MINUS:// 负号
		case TK_DEREF:
			return true;               
		default:
			return false;
	}
}

uint32_t is_reg(char *str){
	int i;
	for(i = R_EAX; i <= R_EDI; ++ i) {
		if(strcmp(str, regsl[i]) == 0) return reg_l(i);
	}
	for(i = R_AX; i <= R_DI; ++ i){
		if(strcmp(str, regsw[i]) == 0) return reg_w(i);
	}
	for(i = R_AL; i <= R_BH; ++ i){
		if(strcmp(str, regsb[i]) == 0) return reg_b(i);
	}
	if(strcmp(str, "eip") == 0) return cpu.eip;
	printf("Reg doesn't exit!\n");
	assert(0);
}

uint32_t eval(int p, int q) {
	if (p > q) {
		printf("Wrong: p > q\n");
		assert(0);
	}
	else if(p == q){
		switch (tokens[q].type) {
/*			case TK_EAX:
				return cpu.eax;
			case TK_EBX:
				return cpu.ebx;
			case TK_ECX:
				return cpu.ecx;
			case TK_EDX:
				return cpu.edx;
			case TK_EDI:
				return cpu.edi;
			case TK_ESI:
				return cpu.esi;
			case TK_EBP:														
				return cpu.ebp;							
			case TK_ESP:			
				return cpu.esp;																																													
		 	case TK_EIP:											
				return cpu.eip;*/
			case TK_REG:
				return is_reg(tokens[q].str + 1);
			case TK_DEC:				
				return atoi(tokens[p].str);																																							
		 	case TK_HEX:{											
				uint32_t hexNum = 0;																																										
				sscanf(tokens[p].str, "%x", &hexNum);																																		
				return hexNum;
			}				
			default:
				assert(0);																																								
		}
	}
	else if(check_parentheses(p, q) == true) {
		return eval(p + 1, q - 1);
	}
	else {
		int op, val1, val2;
		op = find_dominated_op(p, q);
		if(op == p){//单目运算
			switch (tokens[op].type) {
				case '~':
					return ~eval(op + 1, q);
				case TK_MINUS:
					return -eval(op + 1, q);
				case '!':
					return !eval(op + 1, q);
				case TK_DEREF:
					return vaddr_read(eval(op + 1, q), 4);
				default:
					assert(0);
			}
		}
		val1 = eval(p, op - 1);
		val2 = eval(op + 1, q);
		switch (tokens[op].type) {//双目运算
			case '+':
				return val1 + val2;
			case '-':
				return val1 - val2;
			case '*':
				return val1 * val2;
			case '/':
				return val1 / val2;
      case '%':
				return val1 % val2;
			case '|':
				return val1 | val2;
			case '&':
				return val1 & val2;
			case '<':
				return val1 < val2;   
			case '>':
				return val1 > val2;										
			case '^':
					return val1 ^ val2;    
			case TK_EQ:
				return val1 == val2;
			case TK_NQ:  
				return val1 != val2;
			case TK_AND:
				return val1 && val2;
			case TK_OR:
				return val1 || val2;
      case TK_LE:
				return val1 <= val2;
			case TK_GE:
				return val1 >= val2;
			case TK_ML:
				return val1 << val2;
			case TK_MR:
				return val1 >> val2;
			default: 
				assert(0);																																					
		}
	}
	return 0;//其实不会执行到这个
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  int i;
	for (i = 0; i < nr_token; i ++) {
		if (tokens[i].type == '*' && (i == 0 || opEvalDeref(tokens[i - 1].type))) {                                  
			tokens[i].type = TK_DEREF;
		}  
	} 

	for (i = 0; i < nr_token; i ++) {
		if (tokens[i].type == '-' && (i == 0 || opEvalMinus(tokens[i - 1].type))) {
			tokens[i].type = TK_MINUS;
		}
	}

  /* TODO: Insert codes to evaluate the expression. */
  
	return eval(0, nr_token - 1);
}
