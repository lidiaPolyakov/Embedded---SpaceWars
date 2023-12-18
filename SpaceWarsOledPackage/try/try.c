#include <stdio.h>
#include <stdlib.h>

typedef struct SHAPE{
    int x;
    int y;
    int length;
    char* type;
    int hit;
}Shape;

typedef struct SHAPES{
    Shape *item;
    struct Shapes* next;
}Shapes;


int main(){
    Shapes* shapes = (Shapes*)malloc(sizeof(Shapes));
    Shape shape; //(Shape*)malloc(sizeof(Shape));
        //shape->length = 3;
        shape.x = 10;
        shapes->item = *shape;

        printf("%d", shapes->item->x);
//        shapes->item.y = 10;
//        shapes->item.type = "falling";
//        shapes->item.hit = 0;
}