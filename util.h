#ifndef KAOS_UTIL_H
#define KAOS_UTIL_H

#include "f_util.h"

int fd_in_array(FIL *fd, FIL *array[], int len)
{
  for (int i = 0; i < len; i++)
  {
    if (array[i] == fd)
      return 1;
  }
  return 0;
}
int add_fd_to_array(FIL *fd, FIL *array[], int len)
{
  for (int i = 0; i < len; i++)
  {
    if (array[i] == 0)
    {
      array[i] = fd;
      return 1;
    }
  }
  return 0;
}

int remove_fd_from_array(FIL *fd, FIL *array[], int len)
{
  for (int i = 0; i < len; i++)
  {
    if (array[i] == fd)
    {
      array[i] = 0;
      if (i + 1 >= len) // last element
        return 1;
      if (array[i + 1] != 0)
      {
        int j;
        for (j = i + 1; j < len; j++)
        {
          array[j - 1] = array[j];
          if (array[j] == 0)
            break;
        }
        j--;
        if (j == len)
          array[j] = 0;
      }
      return 1;
    }
  }
  return 0;
}

char create_sense_bitmask(FIL *array[], int len)
{
  char ret = 0;
  for (int i = 0; i < len; i++)
  {
    if (array[i] == 0)
      break;
    ret |= 1 << i*2;
  }
  return ret;
}
#endif // KAOS_UTIL_H
