#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
//#include "elf.h"
//#include "elf.c"

//#define max_string_long 32
//#define max_token_num 32
enum {
	NOTYPE = 256, EQ,NUMBER,REGISTER,MINUS,SNUMBER,NEQ,AND,OR,POINTER,MARK

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
	int pri;
} rules[] = {

	/* TODO: Add more rules.
	* Pay attention to the precedence level of different rules.
	*/
	
	{"!=",NEQ,3},//not equal
	{"\\(",'(',7},
	{"\\)",')',7},
	{"\\*",'*',5},//multi
	{"/",'/',5},//divide
	{"-",'-',4},//sub
	{"\\$[eE][a|c|d|b|s|p][x|p|i]",REGISTER,0},
        {"-",'-',4},//sub		//register
	{"0x[a-f0-9]+",SNUMBER,0},
	{"[0-9]+",NUMBER,0},                               //number
	{" +",	NOTYPE,0},				// spaces
	{"\\+", '+',4},					// plus
	{"==", EQ},
	{"!=",NEQ,3},//not equal
        {"&&",AND,2},
	{"\\|\\|",OR,1},
	{"\\b[a-zA-Z_0-9]+" , MARK , 0},		// mark						// equal
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
uint32_t get_addr_from_mark(char *mark);
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
	int pri;
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;//count the number

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				//Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				* to record the token in the array `tokens'. For certain types
				* of tokens, some extra actions should be performed.
				*/

				switch(rules[i].token_type) {
					case NOTYPE :
						break;
					default:
						tokens[nr_token].type=rules[i].token_type;
						tokens[nr_token].pri=rules[i].pri;
						strncpy(tokens[nr_token].str,substr_start,substr_len);
//						printf("%x %d\n",tokens[nr_token].str[0],nr_token);
						tokens[nr_token].str[substr_len]='\0';
						nr_token++;
						break;
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true;
}

int dominant_operator (int start,int end){
	int i,j;
	int min = 10;
	int op= start;
	for (i = start; i <= end;i ++){
		if (tokens[i].type == NUMBER || tokens[i].type == SNUMBER || tokens[i].type == REGISTER ||tokens[i].type ==MARK)
			continue;
		int count = 0;
		bool key = true;
		for (j = i - 1; j >=start ;j --){
			if (tokens[j].type == '(' && !count){
				key = false;
				break;
			}
			if (tokens[j].type == '(')
				count--;
			if (tokens[j].type == ')')
				count++;
		}
		if (!key)continue;
		if (tokens[i].pri <= min){
			min = tokens[i].pri;
			op= i;
		}
 	}
	return op;
}

bool check_parentheses(int start,int end){
	if(tokens[start].type=='('&&tokens[end].type==')'){
		int i;
		int count=0;
		for(i=start+1;i<end;i++){
			if(tokens[i].type=='(')
				count++;
			if(tokens[i].type==')')
				count--;
			if(count<0){
				printf("hh\n");
				Assert(0,"WRONG EXPRESSION");
			}
		}
		if(count!=0){
			printf("aa\n");
			Assert(0,"WRONG EXPRESSION");	
		}
		return true;
	}
	return false;
}

uint32_t eval(int start,int end){
	if(start>end){
		Assert(start>end,"cound not find token\n");
	}
	int i=0;
	for(i=0;i<nr_token;i++){
		if(tokens[i].type=='-'&&(i==0||tokens[i-1].type=='(')){
			tokens[i].type=MINUS;
		}
	}//judge whether it's minus or sub
	if(start==end){
		//printf("hhh\n");
		uint32_t ans;
		if(tokens[start].type==NUMBER){
//			printf("aaa %x %d\n",tokens[start].str[0],start);
			sscanf(tokens[start].str,"%d",&ans);
		}
		if(tokens[start].type==SNUMBER){
			sscanf(tokens[start].str,"%x",&ans);
		}
		//printf("%d\n",ans);
		if(tokens[start].type==REGISTER){
			if(tokens[start].str[2]=='a'&&tokens[start].str[3]=='x')
				ans = cpu.eax;
			if(tokens[start].str[2]=='c'&&tokens[start].str[3]=='x')
				ans = cpu.ecx;
			if(tokens[start].str[2]=='d'&&tokens[start].str[3]=='x')
				ans = cpu.edx;
			if(tokens[start].str[2]=='b'&&tokens[start].str[3]=='x')
				ans = cpu.ebx;
			if(tokens[start].str[2]=='s'&&tokens[start].str[3]=='p')
				ans = cpu.esp;
			if(tokens[start].str[2]=='b'&&tokens[start].str[3]=='p')
				ans = cpu.ebp;
			if(tokens[start].str[2]=='s'&&tokens[start].str[3]=='i')
				ans = cpu.ebp;
			if(tokens[start].str[2]=='d'&&tokens[start].str[3]=='i')
				ans = cpu.edi;
		}
		if (tokens[start].type == MARK)
		{
			ans=get_addr_from_mark(tokens[start].str);
		}
		return ans;

 	}
	else if(check_parentheses(start,end) == true) {
		//printf("hh\n");
		return eval(start+1,end-1);
	}
	else{
		int op=dominant_operator(start,end);
		if(start==op||tokens[op].type==MINUS||tokens[op].type==POINTER||tokens[op].type=='!'){
			uint32_t ans=eval(start+1,end);
			if(tokens[op].type==MINUS)
				return -ans;
			if(tokens[op].type=='!')
                                return !ans;
			if(tokens[op].type==POINTER)
                                return swaddr_read(ans,4,R_DS);


		}
		uint32_t ans1,ans2;
		ans1=eval(start,op-1);
		ans2=eval(op+1,end);
		switch(tokens[op].type){
			case '+':return ans1+ans2;
			case '-':return ans1-ans2;
			case '*':return ans1*ans2;
			case '/':return ans1/ans2;
			case EQ:return ans1==ans2;
                        case NEQ:return ans1!=ans2;
                        case AND:return ans1&&ans2;
                        case OR:return ans1||ans2;
			default:break;
		}


	return 0;
	}
}

		
uint32_t expr(char *e, bool *success){
	if(!make_token(e)) {
		*success = false;
		return 0;
	}
	else{
		*success = true;
	}
	int i;
	for (i = 0;i < nr_token; i ++) {
 		if (tokens[i].type=='*'&&(i==0||(tokens[i - 1].type!=NUMBER&&tokens[i - 1].type!=SNUMBER&&tokens[i - 1].type!= REGISTER&&tokens[i-1].type!=MARK&&tokens[i - 1].type !=')'))) {
			tokens[i].type = POINTER;
			tokens[i].pri = 6;
		}
		if (tokens[i].type =='-'&&(i ==0|| (tokens[i - 1].type != NUMBER && tokens[i - 1].type != SNUMBER && tokens[i - 1].type != REGISTER&&tokens[i-1].type!=MARK&&tokens[i - 1].type !=')'))) {
			tokens[i].type = MINUS;
			tokens[i].pri = 6;
 		}
  	}
	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	return eval(0,nr_token-1);
}


