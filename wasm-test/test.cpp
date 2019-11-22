
#include <stdio.h>

#include <emscripten.h>



EM_JS(void, two_alerts, (), {
    alert('hai');
    alert('bai');
});


int main()
{
    printf("main()\n");

    two_alerts();
}


int asciicker(int i)
{
    return printf("i=%d\nWell done.\n",i);
}

