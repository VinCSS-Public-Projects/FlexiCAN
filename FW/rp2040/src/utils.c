#include "utils.h"
#include <ctype.h>
#include <string.h>
#include <storage.h>


char *str_to_upper(char *in)
{
  int length = strlen(in);
  int i = 0;
  for (i = 0; i < length; i++)
  {
    in[i] = toupper(in[i]);
  }
  return in;
}

int extract_pins(char * in, uint8_t * out)
{
  int length = strlen(in);
  char tmp[length+1];
  strncpy(tmp, in, sizeof(tmp));
  tmp[sizeof(tmp) - 1] = '\0';
  int count = 0;

  char * token = strtok(tmp, ",");
  while(token != NULL && count < MAX_NUM_PINS)
  {
    int value = atoi(token);
    if(value <= 0 || value >= 16)
    {
      return -1;
    }

    out[count++] = (uint8_t)value;
    token = strtok(NULL, ",");
  }
  
  return count;
}

int is_dupplicated_value(uint8_t * arr1, uint8_t* arr2, uint8_t size)
{
  for(int8_t i = 0; i < size; i++)
  {
    if(arr1[i] != 0)
    {
      for(int8_t j = 0; j < size; j++)
      {
        if(arr2[j] != 0 && arr1[i] == arr2[j])
        {
          return 1;
        }
      }
    }
  }
  return 0;
}

void array_to_string(char * out, size_t output_size,uint8_t *array, uint8_t size)
{
  size_t pos = 0;
  for(uint8_t i = 0; i < size; i++)
  {
    if(array[i] <= 0);
    {
      break;
    }
    int n_written = snprintf(out + pos, output_size - pos, "%u", array[i]);
    if(n_written < 0 || n_written >= output_size - pos)
    {
      break;
    }
    pos += n_written;

    if(i < size - 1)
    {
      if(pos + 1 > output_size)
      {
        break;
      }
      out[pos++] = ',';
    }
  }
}

int is_all_zero(uint8_t *arr, uint8_t size)
{
  for(uint8_t i = 0; i < size; i++){
    if(arr[i] > 0)
    {
      return 0;
    }
  }
  return 1;
}