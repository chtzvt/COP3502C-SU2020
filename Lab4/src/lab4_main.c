#include <stdio.h>


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

int main() {
        char valid1[] = "[ A * {B + (C + D)}]";
        char valid2[] = "[{()()}]";
        char inv1[] =  "[ A * {B + (C + D})]";
        char inv2[] = "[ { ( ] ) ( ) }";

        printf("%s is balanced? %d\n", valid1, checkBalance(valid1));
        printf("%s is balanced? %d\n", valid2, checkBalance(valid2));
        printf("%s is balanced? %d\n", inv1, checkBalance(inv1));
        printf("%s is balanced? %d\n", inv2, checkBalance(inv2));

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

                if (empty(&s))
                        return 0;

                char tmp;
                switch (exp[i]) {
                case ')':
                        tmp = top(&s);
                        pop(&s);
                        if (tmp=='{' || tmp=='[')
                                return 0;
                        break;

                case '}':
                        tmp = top(&s);
                        pop(&s);
                        if (tmp=='(' || tmp=='[')
                                return 0;
                        break;

                case ']':
                        tmp = top(&s);
                        pop(&s);
                        if (tmp =='(' || tmp == '{')
                                return 0;
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
        return (priority(ch) != -1) ? 0 : 1;
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
