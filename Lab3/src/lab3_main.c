// Lab 3
// Charlton Trezevant - Summer 2020

// Stores one node of the linked list.
struct node {
    int data;
    struct node* next;
};

// Stores our queue.
struct queue {
    struct node* front;
    struct node* back;
};


void init(struct queue *q);
void enqueue(struct queue *q, int val);
void dequeue(struct queue *q);
struct node *front(struct queue *q);
int empty(struct queue *q);

int main(void){
  return 0; //stub
}
