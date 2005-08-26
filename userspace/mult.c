
#define ARRAY_SIZE 10000
#define POS(x,y) (x*ARRAY_SIZE+y)

int a[ARRAY_SIZE];
int axa[ARRAY_SIZE * ARRAY_SIZE];

int main()
{
  int pos, num = 0;
  int x, y, mult_pos = 0;

  for (num = 0; num < ARRAY_SIZE; ++num)
  {
    a[num] = num;
  }

  for (x = 0; x < ARRAY_SIZE; ++x)
  {
    for (y = 0; y < ARRAY_SIZE; ++y)
    {
      pos = POS(x,y);
      axa[pos]= 0;

      for (mult_pos = 0; mult_pos < ARRAY_SIZE; ++mult_pos)
      {
        axa[pos] = a[mult_pos] *  a[y];
      }
    }
  }
}
