/*
  COP 3502C - Summer 2020
  Charlton Trezevant
  Lab 8 - BSTs
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct tree_node {
  char data[20];
  struct tree_node *left;
  struct tree_node *right;
};

struct tree_node *create_node(char *val);
void inorder(struct tree_node *current_ptr);
struct tree_node *insert(struct tree_node *root, struct tree_node *element);
int find(struct tree_node *current_ptr, char *val);
struct tree_node *parent(struct tree_node *root, struct tree_node *node);
struct tree_node *minVal(struct tree_node *root);
struct tree_node *maxVal(struct tree_node *root);
int isLeaf(struct tree_node *node);
int hasOnlyLeftChild(struct tree_node *node);
int hasOnlyRightChild(struct tree_node *node);
struct tree_node *findNode(struct tree_node *current_ptr, char *value);
struct tree_node *delete (struct tree_node *root, char *value);
int numnodes(struct tree_node *root);

int totalchars(struct tree_node *current_ptr);
int height(struct tree_node *root);
char *biggest(struct tree_node *root);

int main() {
  struct tree_node *root; //very important line. Otherwise all function will fail
  root = NULL;            // suppress unused variable warning
  char ch[50], tmp[20];
  int num_words;

  FILE *infile = fopen("in.txt", "r");
  if (infile == NULL) {
    printf("failed to open input file\n");
    return 1;
  }

  if (!feof(infile)) {
    fscanf(infile, "%d", &num_words);
    fscanf(infile, "%s", tmp);
    root = create_node(tmp);
  } else {
    printf("invalid input file format: number of words unknown\n");
    return 1;
  }

  for (int i = 0; i < num_words; i++) {
    char tmp[20];
    if (!feof(infile)) {
      fscanf(infile, "%s", tmp);
      insert(root, create_node(tmp));
    }
  }

  printf("Inorder traversal:\n\t");
  inorder(root);
  printf("\n\nTree contains %d characters\n", totalchars(root));
  printf("Tree height is: %d\n", height(root));
  printf("Bigliest word is: %s\n", biggest(root));

  while (1) {
    printf("\nSearching in the tree (enter \"exit\" to exit): ");
    scanf("%s", ch);

    if (strlen(ch) > 0) {
      if (strcmp("exit", ch) == 0) {
        printf("Goodbye!\n");
        goto loop_exit;
      }

      if (find(root, ch) == 1) {
        printf("'%s' is contained in the tree!\n", ch);
      } else {
        printf("'%s' is not contained in the tree.\n", ch);
      }
    }

    ch[0] = '\0';
  }

loop_exit:;

  return 0;
}

char *biggest(struct tree_node *root) {
  if (root == NULL)
    return NULL;

  if (root->right == NULL)
    return root->data;

  return biggest(root->right);
}

int height(struct tree_node *root) {
  int left, right;
  if (root == NULL)
    return 0;

  left = height(root->left);
  right = height(root->right);

  if (left > right)
    return left + 1;

  return right + 1;
}

int totalchars(struct tree_node *current_ptr) {
  if (current_ptr != NULL)
    return totalchars(current_ptr->left) + strlen(current_ptr->data) + totalchars(current_ptr->right);
  else
    return 0;
}

void inorder(struct tree_node *current_ptr) {
  // Only traverse the node if it's not null.
  if (current_ptr != NULL) {
    inorder(current_ptr->left);       // Go Left.
    printf("%s ", current_ptr->data); // Print the root.
    inorder(current_ptr->right);      // Go Right.
  }
}

struct tree_node *insert(struct tree_node *root,
                         struct tree_node *element) {

  // Inserting into an empty tree.
  if (root == NULL) {
    return element;
  } else {

    // element should be inserted to the right.
    if (strcmp(element->data, root->data) > 0) {

      // There is a right subtree to insert the node.
      if (root->right != NULL)
        root->right = insert(root->right, element);

      // Place the node directly to the right of root.
      else
        root->right = element;
    }

    // element should be inserted to the left.
    else {

      // There is a left subtree to insert the node.
      if (root->left != NULL)
        root->left = insert(root->left, element);

      // Place the node directly to the left of root.
      else
        root->left = element;
    }

    // Return the root pointer of the updated tree.
    return root;
  }
}

struct tree_node *create_node(char *val) {

  // Allocate space for the node, set the fields.
  struct tree_node *temp;
  temp = (struct tree_node *)malloc(sizeof(struct tree_node));
  strcpy(temp->data, val);
  temp->left = NULL;
  temp->right = NULL;

  return temp; // Return a pointer to the created node.
}

int find(struct tree_node *current_ptr, char *val) {

  // Check if there are nodes in the tree.
  if (current_ptr != NULL) {

    // Found the value at the root.
    if (strcmp(current_ptr->data, val) == 0)
      return 1;

    // Search to the left.
    if (strcmp(current_ptr->data, val) > 0)
      return find(current_ptr->left, val);

    // Or...search to the right.
    else
      return find(current_ptr->right, val);

  } else
    return 0;
}

// Returns the parent of the node pointed to by node in the tree rooted at
// root. If the node is the root of the tree, or the node doesn't exist in
// the tree, null will be returned.
struct tree_node *parent(struct tree_node *root, struct tree_node *node) {

  // Take care of NULL cases.
  if (root == NULL || root == node)
    return NULL;

  // The root is the direct parent of node.
  if (root->left == node || root->right == node)
    return root;

  // Look for node's parent in the left side of the tree.
  if (strcmp(node->data, root->data) < 0)
    return parent(root->left, node);

  // Look for node's parent in the right side of the tree.
  else if (strcmp(node->data, root->data) > 0)
    return parent(root->right, node);

  return NULL; // Catch any other extraneous cases.
}

// Returns a pointer to the node storing the minimum value in the tree
// with the root, root. Will not work if called with an empty tree.
struct tree_node *minVal(struct tree_node *root) {

  // Root stores the minimal value.
  if (root->left == NULL)
    return root;

  // The left subtree of the root stores the minimal value.
  else
    return minVal(root->left);
}

// Returns a pointer to the node storing the maximum value in the tree
// with the root, root. Will not work if called with an empty tree.
struct tree_node *maxVal(struct tree_node *root) {

  // Root stores the maximal value.
  if (root->right == NULL)
    return root;

  // The right subtree of the root stores the maximal value.
  else
    return maxVal(root->right);
}

// Returns 1 if node is a leaf node, 0 otherwise.
int isLeaf(struct tree_node *node) {

  return (node->left == NULL && node->right == NULL);
}

// Returns 1 iff node has a left child and no right child.
int hasOnlyLeftChild(struct tree_node *node) {
  return (node->left != NULL && node->right == NULL);
}

// Returns 1 iff node has a right child and no left child.
int hasOnlyRightChild(struct tree_node *node) {
  return (node->left == NULL && node->right != NULL);
}

// Returns a pointer to a node that stores value in it in the subtree
// pointed to by current_ptr. NULL is returned if no such node is found.
struct tree_node *findNode(struct tree_node *current_ptr, char *value) {

  // Check if there are nodes in the tree.
  if (current_ptr != NULL) {

    // Found the value at the root.
    if (strcmp(current_ptr->data, value) == 0)
      return current_ptr;

    // Search to the left.
    if (strcmp(value, current_ptr->data) < 0)
      return findNode(current_ptr->left, value);

    // Or...search to the right.
    else
      return findNode(current_ptr->right, value);

  } else
    return NULL; // No node found.
}

// Will delete the node storing value in the tree rooted at root. The
// value must be present in the tree in order for this function to work.
// The function returns a pointer to the root of the resulting tree.
struct tree_node *delete (struct tree_node *root, char *value) {

  struct tree_node *delnode, *new_del_node, *save_node;
  struct tree_node *par;
  char *save_val;

  delnode = findNode(root, value); // Get a pointer to the node to delete.

  par = parent(root, delnode); // Get the parent of this node.

  // Take care of the case where the node to delete is a leaf node.
  if (isLeaf(delnode)) { // case 1

    // Deleting the only node in the tree.
    if (par == NULL) {
      free(root); // free the memory for the node.
      return NULL;
    }

    // Deletes the node if it's a left child.
    if (value < par->data) {
      free(par->left); // Free the memory for the node.
      par->left = NULL;
    }

    // Deletes the node if it's a right child.
    else {
      free(par->right); // Free the memory for the node.
      par->right = NULL;
    }

    return root; // Return the root of the new tree.
  }

  // Take care of the case where the node to be deleted only has a left
  // child.
  if (hasOnlyLeftChild(delnode)) {

    // Deleting the root node of the tree.
    if (par == NULL) {
      save_node = delnode->left;
      free(delnode);    // Free the node to delete.
      return save_node; // Return the new root node of the resulting tree.
    }

    // Deletes the node if it's a left child.
    if (value < par->data) {
      save_node = par->left;       // Save the node to delete.
      par->left = par->left->left; // Readjust the parent pointer.
      free(save_node);             // Free the memory for the deleted node.
    }

    // Deletes the node if it's a right child.
    else {
      save_node = par->right;        // Save the node to delete.
      par->right = par->right->left; // Readjust the parent pointer.
      free(save_node);               // Free the memory for the deleted node.
    }

    return root; // Return the root of the tree after the deletion.
  }

  // Takes care of the case where the deleted node only has a right child.
  if (hasOnlyRightChild(delnode)) {

    // Node to delete is the root node.
    if (par == NULL) {
      save_node = delnode->right;
      free(delnode);
      return save_node;
    }

    // Delete's the node if it is a left child.
    if (value < par->data) {
      save_node = par->left;
      par->left = par->left->right;
      free(save_node);
    }

    // Delete's the node if it is a right child.
    else {
      save_node = par->right;
      par->right = par->right->right;
      free(save_node);
    }
    return root;
  }
  //if your code reaches hear it means delnode has two children
  // Find the new physical node to delete.
  new_del_node = minVal(delnode->right);
  save_val = new_del_node->data;

  delete (root, save_val); // Now, delete the proper value.

  // Restore the data to the original node to be deleted.
  strcpy(delnode->data, save_val);

  return root;
}

// Returns the number of nodes in the tree pointed to by root.
int numnodes(struct tree_node *root) {

  if (root == NULL)
    return 0;
  return 1 + numnodes(root->left) + numnodes(root->right);
}
