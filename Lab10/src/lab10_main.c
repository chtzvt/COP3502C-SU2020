/*
  COP 3502C - Summer 2020
  Charlton Trezevant
  Lab 8 - BSTs
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int isHeap_recursive(int arr[], int i, int n);
int isHeap_iter(int arr[], int n);
void print_array(int arr[], int n);

int main() {

  int arr1[] = {12, 13, 14, 15, 110, 115};
  int arr2[] = {12, 110, 14, 15, 13, 115};

  print_array(arr1, 6);
  printf("Is it a minheap? %s\n", ((isHeap_recursive(arr1, 0, 6) == 0) && (isHeap_iter(arr1, 6) == 0)) ? "yes!" : "no");

  print_array(arr2, 6);
  printf("Is it a minheap? %s\n", ((isHeap_recursive(arr2, 0, 6) == 0) && (isHeap_iter(arr2, 6) == 0)) ? "yes!" : "no");
}

void print_array(int arr[], int n) {
  printf("[");
  for (int i = 0; i < n - 1; i++) {
    printf("%d, ", arr[i]);
  }
  printf("%d]\n", arr[n - 1]);
}

int isHeap_recursive(int arr[], int i, int n) {
  if (i > (n - 2) / 2)
    return 0;

  if (arr[i] >= arr[2 * i + 1] && isHeap_recursive(arr, 2 * i + 1, n) &&
      arr[i] >= arr[2 * i + 2] && isHeap_recursive(arr, 2 * i + 2, n))
    return 0;

  return 1;
}

int isHeap_iter(int arr[], int n) {
  for (int i = (n / 2 - 1); i-- >= 0;) {
    if (arr[i] > arr[2 * i + 1])
      return 0;

    if (n > 2 * i + 2)
      if (arr[2 * i + 2] < arr[i])
        return 0;
  }

  return 1;
}
