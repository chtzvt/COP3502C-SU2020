#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// MultiStack configuration opts
#define SIZE 10
#define EMPTY -1


struct stack {
        char items[SIZE];
        int top;
};

void initialize(struct stack* stackPtr);
int full(struct stack* stackPtr);
int push(struct stack* stackPtr, char value);
int empty(struct stack* stackPtr);
char pop(struct stack* stackPtr);
char top(struct stack* stackPtr);
void display(struct stack* stackPtr);

int checkBalance(char exp[]);
char* infixToPostfix(char infix[]);

int main() {

        char* balanceTests[4] = {
                "[ A * {B + (C + D)}]",
                "[{()()}]",
                "[ A * {B + (C + D})]",
                "[ { ( ] ) ( ) }"
        };

        for(int i = 0; i < 4; i++)
                printf("%s is balanced? %d\n", balanceTests[i], checkBalance(balanceTests[i]));

        char* infixTests[5] = {
                "(7 - 3) / (2 + 2))",
                "(5+6)*7-8*9",
                "(7 - 3) / (2 + 2)",
                "3+(4*5-(6/7^8)*9)*10",
                "1000 + 2000"
        };

        printf("\n");

        for(int i = 0; i < 5; i++)
                printf("%s in postfix: %s\n", infixTests[i], infixToPostfix(infixTests[i]));

        return 0;
}

int checkBalance(char exp[]){
        struct stack s;
        initialize(&s);

        for (int i=0; exp[i] != '\0'; i++)
        {
                switch(exp[i]) {
                case '(':
                case '[':
                case '{':
                        push(&s, exp[i]);
                        continue;
                }

                switch (exp[i]) {
                case ')':
                        if(top(&s) != '(')
                                return 0;
                        pop(&s);
                        break;

                case '}':
                        if(top(&s) != '{')
                                return 0;
                        pop(&s);
                        break;

                case ']':
                        if(top(&s) != '[')
                                return 0;
                        pop(&s);
                        break;
                }
        }

        return 1;
}


int priority(char ch)
{
        int p;

        switch(ch) {
        case '^':
                p=3;
                break;

        case '*':
        case '/':
                p=2;
                break;

        case '+':
        case '-':
                p=1;
                break;

        default:
                p=-1;
        }

        return p;
}

int isOperator(char ch){
        return priority(ch) != -1;
}

int isParentheses(char ch){
        switch(ch) {
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
                return 0;

        default:
                return 1;
        }
}

void append(char* s, char e){
        for (int i = 0; i < ((sizeof(*s) / sizeof(char)) - 2); i++) {
                if(s[i] == '\0') {
                        s[i] = e;
                        break;
                }
        }
}

char* infixToPostfix(char infix[])
{
        if(checkBalance(infix) == 0)
                return NULL;

        struct stack s;
        initialize(&s);

        char* out = malloc(sizeof(char) * strlen(infix));
        memset(out, '\0', strlen(infix));

        for (int i = 0; infix[i] != '\0'; i++)
        {
                if(isalpha(infix[i]) || isdigit(infix[i]))
                        append(out, infix[i]);

                if (infix[i] == '(')
                        push(&s, infix[i]);


                if (infix[i] == ')')
                {
                        while (!empty(&s) && top(&s) != '(') {
                                append(out, ' ');
                                append(out, pop(&s));
                        }
                        pop(&s);
                }

                if (isOperator(infix[i]))
                {
                        append(out, ' ');

                        if(empty(&s)) {
                                push(&s, infix[i]);
                                continue;
                        }

                        if(priority(infix[i]) >= priority(top(&s))) {
                                push(&s, infix[i]);
                        } else {
                                while(!empty(&s)) {
                                        if(top(&s) == '(')
                                                break;

                                        append(out, pop(&s));
                                        append(out, ' ');
                                }
                                push(&s, infix[i]);
                        }
                }

        }

        while (!empty(&s)) {
                char tmp = top(&s);
                if(tmp == '(')
                {
                        pop(&s);
                        continue;
                }
                if(isOperator(tmp))
                        append(out, ' ');

                append(out, tmp);
                pop(&s);
        }

        return out;
}



//////////////// From MultiStack ////////////////

void initialize(struct stack* stackPtr) {
        stackPtr->top = -1;
}

int push(struct stack* stackPtr, char value) {

        // Check if the stack is full.
        if (full(stackPtr))
                return 0;

        // Add value to the top of the stack and adjust the value of the top.
        stackPtr->items[stackPtr->top+1] = value;
        (stackPtr->top)++;
        return 1;
}

int full(struct stack* stackPtr) {
        return (stackPtr->top == SIZE - 1);
}

int empty(struct stack* stackPtr) {
        return (stackPtr->top == -1);
}

char pop(struct stack* stackPtr) {

        int retval;

        // Check the case that the stack is empty.
        if (empty(stackPtr))
                return EMPTY;

        // Retrieve the item from the top of the stack, adjust the top and return
        // the item.
        retval = stackPtr->items[stackPtr->top];
        (stackPtr->top)--;
        return retval;
}

char top(struct stack* stackPtr) {

        // Take care of the empty case.
        if (empty(stackPtr))
                return EMPTY;

        // Return the desired item.
        return stackPtr->items[stackPtr->top];
}

void display(struct stack* stackPtr) {
        printf("\nPrinting the Current stack...");
        for(int i=0; i<=stackPtr->top; i++)
                printf("%d ", stackPtr->items[i]);
}
