#include <UbigraphAPI.h>
#include <stdio.h>

int main(int const argc, const char ** const argv) 
{
  ubigraph_clear();

  const int N = 20;

  int i;
  for (i=0; i < N; ++i)
    ubigraph_new_vertex_w_id(i);

  for (i=0; i < N; ++i)
  {
    char tbuf[20];
    int r = (int)( i / (float) N * 255);
    sprintf(tbuf, "#%02x%02x%02x", r, 255-r, 255);
    ubigraph_set_vertex_attribute(i, "color", tbuf);

    ubigraph_new_edge(i, (i+1)%(N/2));
  }
}

