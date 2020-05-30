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
node*  insert_front(node *root, int item)
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

//this function takes an item and insert it in the end of the linked list
node* insert_end(node* root, int item)
{
        node *t;
        node *temp;
        //create a new node and fill-up the node
        temp= (node *) malloc(sizeof(node));
        temp->data=item;
        temp->next=NULL;
        if(root==NULL) //if there is no node in the linked list, make temp as the root
                root=temp;
        else //there is an existing linked list and we need to traverse to reach the end node
        {
                t=root; //t is being used to start traversing
                while(t->next!=NULL) //keep going till the end
                        t=t->next;

                t->next=temp; //t is at the last node of the linked list, so add temp after t.
        }
        return root;
}

/*this function deletes the first occurrence of a given item from linked list.
   it returns the updated/original root
 */
node* DelList(node *root, int item)
{
        node *t;
        node *temp;
        if(root==NULL) //if there is no linked list, just return root/null
                return root;
        if(root->data==item) //if root contains the item, remove the current root and change it to the next node
        {
                temp=root; //put existing root to temp so that we can free it. Otherwise, there will be memory leak
                root=root->next; //change the root by the next node of the current root.
                free(temp);
                return root;
        }

        //the code will reach here if the item is not in the root. So, we need to traverse.
        t=root;

        /*keep going until we reach to the end or we find the item.
           note that we look ahead so that we will be one node before the node we will be deleting
         */
        while(t->next!=NULL && t->next->data != item)
                t=t->next;
        if(t->next==NULL) //if the above loop breaks for this reason, it means the item does not exist. So, return from the function
                return root;
        /*if the code reach here, it indicates the loop exited as the item was found
           now, the node we want to delete is in t->next.*/

        temp=t->next; //we will delete t->next. So, putting it in temp
        t->next=t->next->next; //change the t->next to the the next of the node we will be deleting
        free(temp);
        return root;
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
        int ch,ele, del;
        while(1)
        {
                printf("\nMenu: 1. insert at front, 2. reverse list 3. Insert to place 0. exit: ");
                scanf("%d",&ch);
                switch(ch){
                case 0:
                        printf("\nGOOD BYE\n");
                        goto loop_exit;
                        
                case 1:
                        printf("\nEnter an integer to insert at the front: ");
                        scanf("%d",&ele);
                        root = insert_front(root, ele);
                        display(root);

                case 2:
                        printf("\nReversing list... ");
                        scanf("%d",&ele);
                        root = insert_end(root, ele);
                        display(root);

                case 3:
                        printf("\nEnter an int to insert at place 0: ");
                        scanf("%d",&del);
                        root=DelList(root, del);
                        display(root);

              }
        }
        
        loop_exit: ;
        
        return 0;
}
