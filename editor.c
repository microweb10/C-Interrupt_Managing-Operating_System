/*
editor para practica de perifericos:

    1) Trabaja sobre un buffer de 1024 caracteres.
       No importa de donde provengan los datos de
       ese buffer
    2) Realiza acciones en funcion de los codigos que
       reciba. Estos codigos pueden ser

        a) codigos ascii imprimibles, del 32 al 126
        b) ordenes
            arriba            1
            abajo             2
            izquierda         3
            derecha           4
            primera linea     5
            ultima linea      6
            primera columna   7
            ultima columna    8
            borrar linea      9
            borrar todo      10
            salvar buffer    11

    3) seria mas correcto que usase el controlador de
       terminal para acceder a pantalla, pero para no
       condicionar el editor a la explicacion sobre el
       controlador de pantalla, y como el acceso es por
       memoria, este acceso se realiza directamente por
       el editor
*/

#include <dos.h>          /* para la macro MK_FP */
#include <conio.h>        /* para getch() */

#define ntab 4            /* espacios que sustituyen a tab */  

char *P=MK_FP(0xb800,0);  /* puntero a pantalla fisica */
char F=3,C=8;             /* coordenadas esquina sup. izq. ventana */
char X=0,Y=0;             /* coordenadas del cursor en la ventana */  
char buffed[1024];        /* buffer para el editor */

/* imprime un caracter, toma como argumentos el caracter, la fila y la columna, en coordenadas de pantalla */
void printp(char car, char f, char c){
    *(P+160*f+2*c)=car;
}

/* imprime un caracter, toma como argumentos el caracter, la fila y la columna, en coordenadas de ventana */
void printv(char car, char f, char c){
    printp(car,f+F+1,c+C+1);
}

/* suprime el cursor software de la posicion f,c en coordenas de ventana */
void cursor_no(char f, char c){
    *(P+160*(f+F+1)+2*(c+C+1)+1)=0x07;
}

/* establece el cursor software en las coordenadas ventana f,c */
void cursor(char f, char c){
    *(P+160*(f+F+1)+2*(c+C+1)+1)=0x70;
}

/* dibuja el marco para el editor */
void marco(){
    char j;
    char *p;
    char ayud1[80]="^+ W:arriba;Z:abajo;A:izquierda;S:derecha;T:primera linea;B:ultima linea";
    char ayud2[80]="^+ X:salir;G:guardar;R:borra atras;  tambien funcionan INTRO, TAB, ATRAS";   
    for(j=0;j<65;++j){
        printp('-',F,C+j);
        printp('-',F+17,C+j);
    }
    for(j=0;j<17;++j){
        printp('.',F+j,C);
        printp('.',F+j,C+64);
    }
    p=ayud1;
    j=0;
    while (*p!='\0'){
        printp(*p,F+18,C+j-3);
        ++p;
        ++j;
    }
    p=ayud2;
    j=0;
    while (*p!='\0'){
        printp(*p,F+19,C+j-3);
        ++p;
        ++j;
    }
}

/* limpia el buffer del editor */
void limpia_buffed(char *p){
    int j;
    for(j=0;j<1024;++j) *(p+j)=' ';
    for(j=1;j<16;++j)   *(p+64*j-1)='\0';
}

/* pasa el buffer del editor a la pantalla */
void print_buffed(char *p){
    int j,k;
    for(j=0;j<16;++j){
        for(k=0;k<63;++k){
            printv(*(p+64*j+k),j,k);
        }
    }
}

/* cursor arriba */
void arriba(){
    if (X>0){
        cursor_no(X,Y);
        --X;
        cursor(X,Y);
    }
}

/* cursor abajo */
void abajo(){
    if (X<15){
        cursor_no(X,Y);
        ++X;
        cursor(X,Y);
    }
}

/* cursor izquierda */
void izquierda(){
    if (Y>0){
        cursor_no(X,Y);
        --Y;
        cursor(X,Y);
    }
}

/* cursor a la derecha */
void derecha(){
    if (Y<62){
        cursor_no(X,Y);
        ++Y;
        cursor(X,Y);
    }
}

/* cursor al principio de la linea */
void principio(){
    cursor_no(X,Y);
    Y=0;
    cursor(X,Y);
}

/* cursor al final de la linea */
void fin(){
    cursor_no(X,Y);
    Y=62;
    cursor(X,Y);
}

/* cursor a la primera linea */
void primera(){
    cursor_no(X,Y);
    X=0;
    cursor(X,Y);
}

/* cursor a la ultima linea */
void ultima(){
    cursor_no(X,Y);
    X=15;
    cursor(X,Y);
}

/* inserta un caracter, actualizando el cursor y el contenido del buffer */
void inserta(char c, char *buffer){
    printv(c,X,Y);
    *(buffer+64*X+Y)=c;
    cursor_no(X,Y);
    if (Y<62){
        ++Y;
    }
    else{
        if (X<15){
            Y=0;
            ++X;
        }
    }
    cursor(X,Y);
    return;
}

/* salir: realiza posibles operaciones que hagan falta, como cambiar el estado del proceso */
void salir(){

}

/* guardar: esta funcion es especifica del modo de almacenamiento, se deja pendiente */
void guardar(){

}

/* el editor en si, leyendo teclas y actuando en consecuencia */
void ed(char c, char *p){
    int j;
    switch (c){
        case 23: arriba();    break;
        case 26: abajo();     break;
        case  1: izquierda(); break;
        case 19: derecha();   break;
        case 17: principio(); break;
        case  5: fin();       break;
        case 20: primera();   break;
        case  2: ultima();    break;
        case 24: salir();     break;
        case  7: guardar();   break;
        case 13: principio();
                 abajo();
                 break;
        case  8: break;
        case 18: izquierda();
                 inserta(' ',p);
                 izquierda();
                 break;
        case  9: for(j=0;j<ntab;++j) inserta(' ',p);
                 break;
        default: if ((c>=32)&&(c<=127)) inserta(c,p);
    }
}

main(){
    char c;

    marco();
    limpia_buffed(buffed);
    print_buffed(buffed);
    cursor(X,Y);

    while ((c=getch())!=24){
        ed(c,buffed); 
    }
    
return;
}

