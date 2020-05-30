/* Tanvir Ahmed
   This code implements some basic operation of singly linked list like inserting in the beginning and end, delete operation, and display operation
 */

#include <stdio.h>
#include <stdlib.h>
typedef struct node {
        int data;
        struct node *next;
}node;

//this function takes an item and insert it in the linked list pointed by root.
node* insert_front(node *root, int item)
{
        node *temp;
        //create a new node and fill-up the node
        temp= (node *) malloc(sizeof(node));
        temp->data=item;
        temp->next=NULL;
        if(root==NULL) //if there is no node in the linked list
                root=temp;
        else //there is an existing linked list, so put existing root after temp
        {
                temp->next = root; //put the existing root after temp
                root = temp; //make the temp as the root!
        }
        return root;

}

node* reverse_list(node* root)
{
        node *cursor = root, *prev = NULL, *next = NULL;

        while(cursor != NULL) {
                next = cursor->next;
                cursor->next = prev;
                prev = cursor;
                cursor = next;
        }

        return prev;
}

void insertToPlace(node* root, int val, int index){
        node *cursor = root, *temp, *prev;
        int i = 0;

        while(cursor != NULL) {
                if(i == index || cursor->next == NULL) {
                        temp = (node *) malloc(sizeof(node));
                        temp->data = val;

                        if(cursor->next != NULL) { // splice into list
                                prev = cursor->next;
                                cursor->next = temp;
                                temp->next = prev;
                        } else {
                                cursor->next = temp; // append to end of list
                        }
                        
                        break;
                }
                
                i++;
                cursor = cursor->next;
        }
}

void display(node* t)
{
        printf("\nPrinting your link list.......");

        while(t!=NULL)
        {
                printf("%d ",t->data);
                t=t->next;
        }

}
int main()
{
        node *root; //very important line. Otherwise all function will fail
        root = NULL; // suppress unused variable warning
        node *t;
        int ch,ele, val, idx;
        while(1)
        {
                printf("\nMenu: 1. insert at front, 2. reverse list 3. Insert to place 0. exit: ");
                scanf("%d",&ch);
                switch(ch) {
                case 0:
                        printf("\nGOOD BYE\n");
                        goto loop_exit;

                case 1:
                        printf("\nEnter an integer to insert at the front: ");
                        scanf("%d",&ele);
                        root = insert_front(root, ele);
                        display(root);
                        break;

                case 2:
                        printf("\nReversing list... ");
                        root = reverse_list(root);
                        display(root);
                        break;

                case 3:
                        printf("\nEnter an index to place the data: ");
                        scanf("%d",&idx);
                        printf("\nEnter an int to insert at index %d: ", idx);
                        scanf("%d",&val);
                        insertToPlace(root, val, idx);
                        display(root);
                        break;

                default:
                        printf("\n%d? What do you expect me to do with %d!?\n", ch, ch);

                }
        }

loop_exit:;

        return 0;
}
