#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <alloc.h>
#include <conio.h>
#include <time.h>

#define KNumTerminales 4
#define KNumBucles 5
#define KNumRegistros 8
#define KNumBloques 20
#define KTamCadNum 7
#define KTamBuffTec 10
#define KTamPila 16
#define KTamOrden 80
#define KTamVecPal 300
#define KTamBytecode 1000
#define KTamBloque 1024
#define KTamPantalla 2000
#define ntab 4


/*********** DEFINICION DE ESTRUCTURAS Y TIPOS ***********/

struct orden {
    char linea[KTamOrden];         /* vector para guardar la linea acutal */
    int p;                         /* puntero de escritura */
};
typedef struct orden ORDEN;

struct pila {
    int valores[KTamPila];         /* vector para guardar los numero de la pila */
    int cima;                      /* cima de la pila (primer hueco libre) */
};
typedef struct pila PILA;

struct pilab {
    int contador[KNumBucles];      /* vector para guardar las repeticiones de cada bucle */
    char* cuerpo[KNumBucles];      /* vector de punteros a los cuerpos de los bucles */
    int bucle;                     /* bucle por el que estamos ejecutando */
};
typedef struct pilab PILAB;

struct bufferTec {
    char buffer[KTamBuffTec];       /* vector para guardar cararacteres */ 
    int s;                          /* semaforo */
    int e;                          /* puntero de escritura */
    int l;                          /* puntero de lectura */
    int t;                          /* tamanyo actual del buffer */
};
typedef struct bufferTec BUFFERTEC;

struct terminal {
    ORDEN orden;                    /* una linea de la pantalla (orden actual) */
    BUFFERTEC bufftec;              /* buffer de teclado */
    PILA pila;                      /* pila para artimetica */
    PILAB pilabucle;                /* pila para bucles */
    char cadnum[KTamCadNum];        /* cadena para en numero introducido por teclado en programa */
    char pantalla[KTamPantalla];    /* pantalla */
    char* palabras[KTamVecPal];     /* punteros a palabras de una cadena de caracteres */
    char bytecode[KTamBytecode];    /* codigo o bytecode */
    char* pbytecode;                /* puntero para la ejecucion del bytecode */
    int fila,columna;               /* cursor de la pantalla */
    int numbloque;                  /* numero de bloque (del disco) que se esta tratando */
    int registro;                   /* registro que queremos modificar/acceder */
    int estado;                     /* estado */
    int reg[KNumRegistros];         /* registros del interprete de bytecode */
};
typedef struct terminal TERMINAL;


/************ VARIABLES GLOBALES *************/

char F=3,C=8;                         /* EDITOR - coordenadas esquina sup. izq. ventana */
char X[KNumTerminales]={0,0,0,0};     /* EDITOR - coordenadas del cursor en la ventana */  
char Y[KNumTerminales]={0,0,0,0};     /* EDITOR - coordenadas del cursor en la ventana */  
char buffed[KNumTerminales][KTamBloque];/* EDITOR - buffer para el editor */

char tablaTec[2][90];                 /* tabla para conversiones (digitoTeclado -> Caracter) */
char* p=(char *)MK_FP(0xb800,0);      /* puntero al primer caracter de la pantalla */
char teclaPulsada;                    /* caracter correspondiente a la tecla pulsada */
char disco[KNumBloques][KTamBloque];  /* disco duro (20 bloques de 1024 bytes) */
int algo=0;                           /* bandera para saber si se ha pulsado alguna tecla */
int may=0;                            /* bandera para saber si el caracter es en mayusculas */
int control=0;                        /* bandera para saber si se ha pulsado la tecla control */
int cad=0;                            /* bandera para saber si lo que se va a imprimir es una cadena */
int terminal;                         /* indicador de terminal (0,1,2,3) */
TERMINAL t[KNumTerminales];           /* terminales (4 terminales) */


/***************** EDITOR DE BLOQUES *****************/

/* imprime un caracter, toma como argumentos el caracter, la fila y la columna, en coordenadas de pantalla */
void printp(char car, char f, char c){
    *(p+160*f+2*c)=car;
}

/* imprime un caracter, toma como argumentos el caracter, la fila y la columna, en coordenadas de ventana */
void printv(char car, char f, char c){
    printp(car,f+F+1,c+C+1);
}

/* suprime el cursor software de la posicion f,c en coordenas de ventana */
void cursor_no(char f, char c){
    *(p+160*(f+F+1)+2*(c+C+1)+1)=0x07;
}

/* establece el cursor software en las coordenadas ventana f,c */
void cursor(char f, char c){
    *(p+160*(f+F+1)+2*(c+C+1)+1)=0x70;
}

/* dibuja el marco para el editor */
void marco(){
    char j;
    char *q;
    char ayud1[80]="    CONTROL+ W->Arriba Z->Abajo A->Izquierda S->Derecha G->Guardar";
    char ayud2[80]="    CONTROL+ T->Primera Linea   B->Ultima Linea         X->Salir";   
    for(j=0;j<65;++j){
        printp('-',F,C+j);
        printp('-',F+17,C+j);
    }
    for(j=0;j<17;++j){
        printp('.',F+j,C);
        printp('.',F+j,C+64);
    }
    q=ayud1;
    j=0;
    while (*q!='\0'){
        printp(*q,F+18,C+j-3);
        ++q;
        ++j;
    }
    q=ayud2;
    j=0;
    while (*q!='\0'){
        printp(*q,F+19,C+j-3);
        ++q;
        ++j;
    }
}

/* borra el marco del editor */
void borra_marco(){
    int i,k;
    char j;
    char *q;
    char ayud1[80]="                                                                  ";
    char ayud2[80]="                                                                  ";   
    for(j=0;j<65;++j){
        printp(' ',F,C+j);
        printp(' ',F+17,C+j);
    }
    for(j=0;j<17;++j){
        printp(' ',F+j,C);
        printp(' ',F+j,C+64);
    }
    q=ayud1;
    j=0;
    while (*q!='\0'){
        printp(*q,F+18,C+j-3);
        ++q;
        ++j;
    }
    q=ayud2;
    j=0;
    while (*q!='\0'){
        printp(*q,F+19,C+j-3);
        ++q;
        ++j;
    }
    for(i=0;i<16;++i){
        for(k=0;k<63;++k){
            printv(' ',i,k);
        }
    }
}

/* limpia el buffer del editor */
void limpia_buffed(char *q){
    int j;
    for(j=0;j<1024;++j) *(q+j)=' ';
    for(j=1;j<16;++j)   *(q+64*j-1)='\0';
}

/* pasa el buffer del editor a la pantalla */
void print_buffed(char *q){
    int j,k;
    for(j=0;j<16;++j){
        for(k=0;k<63;++k){
            printv(*(q+64*j+k),j,k);
        }
    }
}

/* cursor arriba */
void arriba(){
    if (X[terminal]>0){
        cursor_no(X[terminal],Y[terminal]);
        --X[terminal];
        cursor(X[terminal],Y[terminal]);
    }
}

/* cursor abajo */
void abajo(){
    if (X[terminal]<15){
        cursor_no(X[terminal],Y[terminal]);
        ++X[terminal];
        cursor(X[terminal],Y[terminal]);
    }
}

/* cursor izquierda */
void izquierda(){
    if (Y[terminal]>0){
        cursor_no(X[terminal],Y[terminal]);
        --Y[terminal];
        cursor(X[terminal],Y[terminal]);
    }
}

/* cursor a la derecha */
void derecha(){
    if (Y[terminal]<62){
        cursor_no(X[terminal],Y[terminal]);
        ++Y[terminal];
        cursor(X[terminal],Y[terminal]);
    }
}

/* cursor al principio de la linea */
void principio(){
    cursor_no(X[terminal],Y[terminal]);
    Y[terminal]=0;
    cursor(X[terminal],Y[terminal]);
}

/* cursor al final de la linea */
void fin(){
    cursor_no(X[terminal],Y[terminal]);
    Y[terminal]=62;
    cursor(X[terminal],Y[terminal]);
}

/* cursor a la primera linea */
void primera(){
    cursor_no(X[terminal],Y[terminal]);
    X[terminal]=0;
    cursor(X[terminal],Y[terminal]);
}

/* cursor a la ultima linea */
void ultima(){
    cursor_no(X[terminal],Y[terminal]);
    X[terminal]=15;
    cursor(X[terminal],Y[terminal]);
}

/* inserta un caracter, actualizando el cursor y el contenido del buffer */
void inserta(char c, char *buffer){
    printv(c,X[terminal],Y[terminal]);
    *(buffer+64*X[terminal]+Y[terminal])=c;
    cursor_no(X[terminal],Y[terminal]);
    if (Y[terminal]<62){
        ++Y[terminal];
    }
    else{
        if (X[terminal]<15){
            Y[terminal]=0;
            ++X[terminal];
        }
    }
    cursor(X[terminal],Y[terminal]);
}

/* funcion que pone en pantalla fisica la pantalla virtual */
void CargarPantalla(){
    char* punt;
    int i;
    punt=p;
    for(i=0;i<KTamPantalla;i++){
        *punt=t[terminal].pantalla[i];
        punt+=2;
    }
}

/* funcion que pone el estado del terminal a un valor */
void PonerEstado(int nuevoestado){
    t[terminal].estado=nuevoestado;
}

/* salir: realiza posibles operaciones que hagan falta, como cambiar el estado del proceso */
void salir(){
    cursor_no(X[terminal],Y[terminal]);
    X[terminal]=0, Y[terminal]=0;
    CargarPantalla();
    t[terminal].estado=0;
}

/* guardar: esta funcion es especifica del modo de almacenamiento, se deja pendiente */
void guardar(){
    int i;
    for(i=0;i<KTamBloque;i++)
        disco[t[terminal].numbloque-1][i]=buffed[terminal][i];
}

/* el editor en si, leyendo teclas y actuando en consecuencia */
void ed(char c, char *q){
    int j;
    switch (c){
        case 23: arriba();    break;
        case 26: abajo();     break;
        case  3: izquierda(); break;
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
        case 18: izquierda();
                 inserta(' ',q);
                 izquierda();
                 break;
        case  9: for(j=0;j<ntab;++j) inserta(' ',q);
                 break;
        default: if(c!=0) inserta(c,q);
    }
}


/*********** FUNCIONES AUXILIARES ***********/

/* funcion que inicializa la tabla de conversiones (codigo de teclado -> caracter correspondiente */
void InicTabla(){
    /* caracteres en minuscula */
        tablaTec[0][2]='1';tablaTec[0][3]='2';tablaTec[0][4]='3';tablaTec[0][5]='4';tablaTec[0][6]='5';
        tablaTec[0][7]='6';tablaTec[0][8]='7';tablaTec[0][9]='8';tablaTec[0][10]='9';tablaTec[0][11]='0';
        tablaTec[0][86]='<';tablaTec[0][15]=9; tablaTec[0][71]=17; tablaTec[0][79]=5; /* tab, inicio y fin*/
        tablaTec[0][16]='q';tablaTec[0][17]='w';tablaTec[0][18]='e';tablaTec[0][19]='r';tablaTec[0][20]='t';
        tablaTec[0][21]='y';tablaTec[0][22]='u';tablaTec[0][23]='i';tablaTec[0][24]='o';tablaTec[0][25]='p';
        tablaTec[0][27]='+';tablaTec[0][30]='a';tablaTec[0][31]='s';tablaTec[0][32]='d';tablaTec[0][33]='f';
        tablaTec[0][34]='g';tablaTec[0][35]='h';tablaTec[0][36]='j';tablaTec[0][37]='k';tablaTec[0][38]='l';
        tablaTec[0][39]='ñ';tablaTec[0][44]='z';tablaTec[0][45]='x';tablaTec[0][46]='c';tablaTec[0][47]='v';
        tablaTec[0][48]='b';tablaTec[0][49]='n';tablaTec[0][50]='m';tablaTec[0][51]=',';tablaTec[0][52]='.';
        tablaTec[0][53]='-';tablaTec[0][57]=' ';tablaTec[0][28]=13; tablaTec[0][14]=18; tablaTec[0][1]=1;
                                                  /* Intro */         /* Retroceso */       /* ESC */
    /* CARACTERES EN MAYUSCULA */
        tablaTec[1][2]='!';tablaTec[1][3]='"';tablaTec[1][4]='·';tablaTec[1][5]='$';tablaTec[1][6]='%';
        tablaTec[1][7]='&';tablaTec[1][8]='/';tablaTec[1][9]='(';tablaTec[1][10]=')';tablaTec[1][11]='=';
        tablaTec[1][86]='>';tablaTec[1][15]=9; tablaTec[1][71]=17; tablaTec[1][79]=5; /* tab, inicio y fin*/
        tablaTec[1][16]='Q';tablaTec[1][17]='W';tablaTec[1][18]='E';tablaTec[1][19]='R';tablaTec[1][20]='T';
        tablaTec[1][21]='Y';tablaTec[1][22]='U';tablaTec[1][23]='I';tablaTec[1][24]='O';tablaTec[1][25]='P';
        tablaTec[1][27]='*';tablaTec[1][30]='A';tablaTec[1][31]='S';tablaTec[1][32]='D';tablaTec[1][33]='F';
        tablaTec[1][34]='G';tablaTec[1][35]='H';tablaTec[1][36]='J';tablaTec[1][37]='K';tablaTec[1][38]='L';
        tablaTec[1][39]='Ñ';tablaTec[1][44]='Z';tablaTec[1][45]='X';tablaTec[1][46]='C';tablaTec[1][47]='V';
        tablaTec[1][48]='B';tablaTec[1][49]='N';tablaTec[1][50]='M';tablaTec[1][51]=';';tablaTec[1][52]=':';
        tablaTec[1][53]='_';tablaTec[1][57]=' ';
}

/* funcion que inicializa los datos */
void InicEstructuras(){
    int i,j;
    for (i=0;i<KNumTerminales;i++){               /* los terminales */
        for(j=0;j<KTamOrden;j++) t[i].orden.linea[j]=' ';            /* una linea de la pantalla (Orden actual) */
        t[i].orden.p=0;
        for(j=0;j<KTamPantalla;j++) t[i].pantalla[j]=' ';            /* pantalla */      
        t[i].fila=0; t[i].columna=0;                                 /* cursor de la pantalla */
        for(j=0;j<KTamBuffTec;j++) t[i].bufftec.buffer[j]=' ';       /* buffer de teclado */
        t[i].bufftec.e=0; t[i].bufftec.l=0;
        t[i].bufftec.s=0; t[i].bufftec.t=0;              
        for(j=0;j<KTamPila;j++) t[i].pila.valores[j]=0;              /* pila para artimetica */
        t[i].pila.cima=0;
        for(j=0;j<KTamBytecode;j++) t[i].bytecode[j]=' ';            /* codigo o bytecode */
        t[i].estado=0;                                               /* estado */
        t[i].pbytecode=NULL;                                         /* puntero para la ejecucion del bytecode */
        t[i].registro=0;                                             /* registro incial 0 */
        t[i].numbloque=0;                                            /* numbloque incial 0 */
        for(j=0;j<KNumRegistros;j++) t[i].reg[j]=0;                  /* registros  */
        for(j=0;j<KTamCadNum;j++) t[i].cadnum[j]=' ';                /* cadena para el numero introducido en programa */
        for(j=0;j<KNumBucles;j++) t[i].pilabucle.contador[j]=0;      /* pila para bucles */
        for(j=0;j<KNumBucles;j++) t[i].pilabucle.cuerpo[j]=NULL; 
        t[i].pilabucle.bucle=0;
    }
}

/* funcion que desactiva el cursor hardware */
void DesactivarCursorHW(){
    char byte;
    outportb(0x3d4,0xa);
    byte=inportb(0x3d5);
    byte = 255;
    outportb(0x3d4,0xa);
    outportb(0x3d5,byte);
}


/* funcion que guarda la pantalla fisica en la pantalla virtual */
void SalvarPantalla(){
    char* punt;
    int i;
    punt=p;
    for(i=0;i<KTamPantalla;i++){
        t[terminal].pantalla[i]=*punt;
        punt+=2;
    }
}

/* funcion que borra la pantalla fisica*/
void BorrarPantalla(){
   char* pos;
   int i;
   pos=p;
   for(i=0;i<4000;i++){
      *pos=' '; pos+=2;
   }
}

/* funcion que sube una linea la pantalla fisica */
void SubirPantalla(){
     int c,tope;
     char *i,*j;
     i=p;
     j=p+160;
     tope=KTamPantalla-80;
     for(c=0;c<tope;c++){
         *i=(*j);
         i+=2;                    
         j+=2;
     }
     for(c=0;c<80;c++){
         *i=' ';
         i+=2;
     }                 
}

/* funcion que establece el cursor software */
void CursorOn(){
    *(p+160*t[terminal].fila+2*t[terminal].columna)='_';
}

/* funcion que borra el cursor software */
void CursorOff(){
    *(p+160*t[terminal].fila+2*t[terminal].columna)=' ';
}

/* funcion que borra la ultima tecla en el terminal */
void Retroceso(){
    if(t[terminal].columna>0){ 
        t[terminal].columna--;
        *(p+160*t[terminal].fila+2*t[terminal].columna)=' ';
    }
}

/* funcion que realiza un retorno de carro en el terminal */
void Intro(){ 
    if(t[terminal].fila==24) SubirPantalla();
    else t[terminal].fila++;
    t[terminal].columna=0;
}

/* funcion que escribe el caracter en la pantalla */
void Escribe(char caracter){
     *(p+160*t[terminal].fila+2*t[terminal].columna)=caracter;
     t[terminal].columna=(t[terminal].columna+1)%80;
     if (t[terminal].columna==0)
         if(t[terminal].fila==24) SubirPantalla();
         else t[terminal].fila++;
     
}

/* funcion terminal */
void Terminal(char caracter){
    CursorOff();    
    switch(caracter){
            case 18: Retroceso(); break;
            case 13: Intro(); break;
            default: Escribe(caracter); break;
    }
    CursorOn();
}

/* funcion que nos dice si la tecla pulsada es valida o no (1=SI,0=NO,2=CAMBIO_TERMINAL) */
int TeclaValida(int num){
    if(((num>=1)&&(num<=11))||((num>=14)&&(num<=25))||((num>=30)&&(num<=39))||((num>=44)&&(num<=53))||(num==57)||(num==71)||(num==79)||(num==27)||(num==28)||(num==86))
        return 1;                                /* digitos, letras, signos aritmeticos, intro y retroceso */
    if((num==54)||(num==42)){may=1; return 0;}   /* Mayusculas Pulsada */
    if((num==182)||(num==170)){may=0; return 0;} /* Mayusculas Liberada */
    if(num==29){control=1; return 0;}            /* Control Pulsada */
    if(num==157){control=0; return 0;}           /* Control Liberada */
    if((num>=59)&&(num<=62)) return 2;           /* F1,F2,F3,F4 (Cambio de Terminal) */
return 0;
}

/* funcion que mete un caracter en el buffer de teclado */
void MeterEnBuffer(char caracter){
    while(t[terminal].bufftec.s!=0);      /* esperamos semaforo abierto */
    t[terminal].bufftec.s=1;              /* cerramos semaforo */
    if(t[terminal].bufftec.t<KTamBuffTec){
        t[terminal].bufftec.buffer[t[terminal].bufftec.e]=caracter;
        t[terminal].bufftec.e=(t[terminal].bufftec.e+1)%KTamBuffTec;
        t[terminal].bufftec.t++;
    }
    t[terminal].bufftec.s=0;              /* abrimos semaforo */
}

/* funcion que extrae un caracter del buffer de teclado */
char ExtraerDelBuffer(){
    char res;
    while(t[terminal].bufftec.s!=0);      /* esperamos semaforo abierto */
    t[terminal].bufftec.s=1;              /* cerramos semaforo */
    if(t[terminal].bufftec.t>0){
        res=t[terminal].bufftec.buffer[t[terminal].bufftec.l];
        t[terminal].bufftec.l=(t[terminal].bufftec.l+1)%KTamBuffTec;
        t[terminal].bufftec.t--;
    }
    t[terminal].bufftec.s=0;              /* abrimos semaforo */
return res;
}

/* funcion que transforma los caracteres pulsados con control para el editor */
char Convertir(char caracter){
    char res;
    switch(caracter){
        case 'w': res=23; break;
        case 'z': res=26; break;
        case 'a': res=3; break;
        case 's': res=19; break;
        case 't': res=20; break;
        case 'b': res=2; break;
        case 'x': res=24; break;
        case 'g': res=7; break;
        default: res=0;
    }
return res;
}

/* RUTINA para cuando se pulsa una tecla del teclado */
void interrupt rutinaTeclado(){
    int valor;
    valor=inportb(0x60);
    /*printf("valor numerico de la tecla pulsada = %d\n",valor);*/
    algo=TeclaValida(valor);
    if(algo==2){                         /* Cambio de Terminal */
            if(t[terminal].estado==4) cursor_no(X[terminal],Y[terminal]);
            else SalvarPantalla();
            CursorOff();
            terminal=valor-59;
            CargarPantalla();
            CursorOn();
            if(t[terminal].estado==4) {
                marco();
                print_buffed(buffed[terminal]);
                cursor(X[terminal],Y[terminal]);
            }
    }
    else{
        if(algo==1){
            teclaPulsada=tablaTec[may][valor];
            if(control==1) teclaPulsada=Convertir(teclaPulsada);
            if((t[terminal].estado==0)||(t[terminal].estado==2)||(t[terminal].estado==4)){ /*si estado es esperando tecla */
                MeterEnBuffer(teclaPulsada);  /* poner en buffer del terminal */
                t[terminal].estado++;         /* poner en estado ejecutandose */
            }
        }
    }
outportb(0x20,0x20);                          /* reiniciar controlador prog interrup */
}

/* funcion que inicializa el vector de palabras a null */
void InicVecPalabras(char* vector[]){
    int i;
    for(i=0;i<KTamVecPal;i++){
        vector[i]=NULL;
    }
}

/* funcion que identificas las distintas palabras de una linea completa */
void IdentPalabras(char* linea, char* palab[]){
    int i=0;          /*contador para la linea entera*/
    int j=0;          /*contador para las palabras*/
    int enc=0;        /*bandera encontrado*/
    int cad=0;        /*bandera cadena de caracteres*/
    InicVecPalabras(palab);
    while((i<KTamBloque)&&(linea[i]!='\0')){
        if(enc==0){
            if(linea[i]!=' '){ /*empieza una palabra*/
                if(linea[i]=='"'){ /*empieza una cadena de caracteres*/
                    cad=1;
                }
                palab[j]=&linea[i];
                j++;
                enc=1;
            }
        }
        else{
            if(cad){                
                if(linea[i]=='"'){  /*termina una cadena de caracteres*/
                    linea[i]='\0';
                    enc=0;
                    cad=0;
                }
            }
            else{
                if(linea[i]==' '){  /*termina una palabra*/
                    linea[i]='\0';
                    enc=0;
                }
            }
        }
        i++;
    }
}

/* funcion que nos dice si un caracter es numerico */
int Num(char caracter){
    int res=0;
    if(((int)caracter>=48)&&((int)caracter<=57)) res=1;
return res;
}

/* funcion que nos dice si dos cadenas son iguales */
int Igual(char* cad1, char* cad2){
    int res=0;
    int i=0;
    while(cad1[i]==cad2[i]){
        if(cad1[i]=='\0') return 1;
        i++;
    }
return res;
}

/* funcion que nos dice si una cadena es un numero */
int EsNumero(char* puntero){
    int res=0;
    if(Igual(puntero,"-")) return res;
    else{
        if((Num(*puntero))||(*puntero=='-')){
            res=1;
            }
        puntero++;
        while((res==1)&&(*puntero!='\0')){
            if(Num(*puntero)==0) res=0;
            puntero++;
        }
    }
return res;
}

/* funcion que convierte una cadena de caracteres a un entero*/
int ConvertirANumero(char* cadena){
    int res; 
    res=atoi(cadena);
return res;
}

/* funcion que convierte un entero a una cadena de caracteres */
void ConvertirACadena(int numero, char cadena[]){
    int cifras=2;
    int neg=0;
    int cociente;
    int resto;
    if (numero<0){
        neg=1;
        numero=numero*(-1);
        cadena[0]='-';
    }
    if(numero<10) cifras--;
    cociente=numero/10;
    while (cociente>=10){
        cifras++;
        cociente=cociente/10;
    }
    cadena[cifras+neg]='\0';
    cifras--;
    cociente=numero;
    while (cociente>=10){
        resto=cociente%10;
        cociente=cociente/10;
        cadena[cifras+neg]=resto+48;
        cifras--;
    }
    cadena[cifras+neg]=cociente+48;
}

void InicializarDisco(){
    
    /*En el DISKETTE tenemos 80 cilindros --- del 0-79)*/
    /*Tenemos 18 sectores en cada cilindro -- del 0 a 17*/
    struct REGPACK reg;
    char *buffer=(char *)malloc(512);
    int cil=0,sect=0;           /*numero de cilindro y sector*/
    int i,j;
        
    reg.r_ax=0x0201;            /*indica que queremos leer de un dispositivo*/
    reg.r_cx= (cil*256) + sect; /*indica que cilindro y sector vamos a leer*/
    reg.r_dx=0x0000;            /*indica que el dispositivo es la disquetera*/
    reg.r_es=FP_SEG(buffer);
    reg.r_bx=FP_OFF(buffer); 
    intr(0x13,&reg);            /* leyendo cilindro, sector */
    
    if(reg.r_ax==0x0001){ /* si no hay ningun error */
        for(j=0;j<KNumBloques;j++){
            reg.r_ax=0x0201;            /*indica que queremos leer de un dispositivo*/
            reg.r_cx= (cil*256) + sect; /*indica que cilindro y sector vamos a leer*/
            reg.r_dx=0x0000;            /*indica que el dispositivo es la disquetera*/
            reg.r_es=FP_SEG(buffer);
            reg.r_bx=FP_OFF(buffer); 
            intr(0x13,&reg);            /* leyendo cilindro, sector */
            
            for(i=0;i<512;i++) disco[j][i] = *(buffer+i);
            sect++;
            
            reg.r_ax=0x0201;            /*indica que queremos leer de un dispositivo*/
            reg.r_cx= (cil*256) + sect; /*indica que cilindro y sector vamos a leer*/
            reg.r_dx=0x0000;            /*indica que el dispositivo es la disquetera*/
            reg.r_es=FP_SEG(buffer);
            reg.r_bx=FP_OFF(buffer); 
            intr(0x13,&reg);            /* leyendo cilindro, sector */
            
            for(i=0;i<512;i++) disco[j][512+i] = *(buffer+i);
            sect++;
            if(sect==18){
                cil++;
                sect=0;
            }
        }
        
    }
    else{ /* si no hay disquetera inicializamos el disco vacio */
        for(i=0;i<KNumBloques;i++)                                        
            for(j=0;j<KTamBloque;j++)
                disco[i][j]=' ';                      
    }
}

void GuardarEnDisco(){
    
    /*En el DISKETTE tenemos 80 cilindros --- del 0-79)*/
    /*Tenemos 18 sectores en cada cilindro -- del 0 a 17*/
    struct REGPACK reg;
    char *buffer=(char *)malloc(512);
    int cil=0,sect=0;           /*numero de cilindro y sector*/
    int i,j;
        
    reg.r_ax=0x0201;            /*indica que queremos leer de un dispositivo*/
    reg.r_cx= (cil*256) + sect; /*indica que cilindro y sector vamos a leer*/
    reg.r_dx=0x0000;            /*indica que el dispositivo es la disquetera*/
    reg.r_es=FP_SEG(buffer);
    reg.r_bx=FP_OFF(buffer); 
    intr(0x13,&reg);            /* leyendo cilindro, sector */
    
    if(reg.r_ax==0x0001){ /* si no hay ningun error */
        for(j=0;j<KNumBloques;j++){
        
            for(i=0;i<512;i++) *(buffer+i)=disco[j][i];
            reg.r_ax=0x0301;            /*indica que queremos escribir en un dispositivo*/
            reg.r_cx= (cil*256) + sect; /*indica que cilindro y sector vamos a escribir*/
            reg.r_dx=0x0000;            /*indica que el dispositivo es la disquetera*/
            reg.r_es=FP_SEG(buffer);
            reg.r_bx=FP_OFF(buffer); 
            intr(0x13,&reg);            /* escribiendo cilindro, sector */
            
            sect++;
            
            for(i=0;i<512;i++) *(buffer+i)=disco[j][512+i];
            reg.r_ax=0x0301;            /*indica que queremos escribir en un dispositivo*/
            reg.r_cx= (cil*256) + sect; /*indica que cilindro y sector vamos a escribir*/
            reg.r_dx=0x0000;            /*indica que el dispositivo es la disquetera*/
            reg.r_es=FP_SEG(buffer);
            reg.r_bx=FP_OFF(buffer); 
            intr(0x13,&reg);            /* escribiendo cilindro, sector */
            
            sect++;
            if(sect==18){
                cil++;
                sect=0;
            }
        }
        
    }
}

/* funcion que nos dice el estado del terminal */
int VerEstado(){
    return t[terminal].estado;
}

/* funcion que nos devuelve el puntero al bytecode */
char* ByteCod(){
    return t[terminal].pbytecode;
}

/* funcion que nos dice el tamaño del buffer de teclado */
int TamBuffTec(){
    return t[terminal].bufftec.t;
}

/* funcion que rellena el numero introducido para un programa*/
void CadNum(int pos,char caracter){
    t[terminal].cadnum[pos]=caracter;
}

/* funcion que nos dice si el numero introducido para un programa es negativo*/
int CadNumNeg(){
    if(t[terminal].cadnum[0]=='-') return 1;
    else return 0;
}

/* funcion que vacia el Bytecode para una compilacion nueva */
void VaciarByteCode(){
    int i;
    for(i=0;i<KTamBytecode;i++) t[terminal].bytecode[i]=' ';
}

/* funcion que carga un bloque del disco al buffer del editor */
void CargarBuffed(){
    int i;
    for(i=0;i<KTamBloque;i++)
        buffed[terminal][i]=disco[t[terminal].numbloque-1][i];
}

/* funcion que transforma la cadena en el bytecode correspondiente */
char BytCod(char *cadena, int *nnum, int *ncad, int *nreg, int *nif, int *nelse, int *nthen, int *ndo, int *nloop, int *nend){
    char res=100;
    char err2[KTamOrden]="WARNING: Se ha encontrado una operacion (+) y no hay valores para calcularla";
    char err3[KTamOrden]="WARNING: Se ha encontrado una operacion (-) y no hay valores para calcularla";
    char err4[KTamOrden]="WARNING: Se ha encontrado una operacion (*) y no hay valores para calcularla";
    char err5[KTamOrden]="WARNING: Se ha encontrado una operacion (/) y no hay valores para calcularla";
    char err6[KTamOrden]="WARNING: Se ha encontrado una operacion (%) y no hay valores para calcularla";
    char err7[KTamOrden]="WARNING: Se ha encontrado una comparacion (=) y no hay valores para calcularla";
    char err8[KTamOrden]="WARNING: Se ha encontrado una comparacion (>) y no hay valores para calcularla";
    char err9[KTamOrden]="WARNING: Se ha encontrado una comparacion (<) y no hay valores para calcularla";
    char err18[KTamOrden]="WARNING: Se ha encontrado una seleccion (if) y no hay valor para comparar";
    char err21[KTamOrden]="WARNING: Se ha encontrado un bucle (do) y no hay valores de inicio y fin";
    char err25[KTamOrden]="WARNING: Se quiere imprimir algo y no hay valor para mostrar";
    char err26[KTamOrden]="WARNING: Se quiere recuperar un registro y este no esta referenciado";
    char err27[KTamOrden]="WARNING: Se quiere almacenar un registro y este no esta referenciado";
    char err28[KTamOrden]="WARNING: Se quiere almacenar un registro y no hay valor para almacenar";
    char err01[KTamOrden]="WARNING: La orden encontrada '";
    char err02[KTamOrden]="' no se puede compilar";
    int i=0;
    if(Igual(cadena,"+")){
        if(*nnum<2){
            while(err2[i]!='\0') Terminal(err2[i++]);
            Terminal(13);
            *nnum=1;
        }
        else (*nnum)--;
        res=2;
        return res;
    }
    if(Igual(cadena,"-")){
        if(*nnum<2){
            while(err3[i]!='\0') Terminal(err3[i++]);
            Terminal(13);
            *nnum=1;
        }
        else (*nnum)--;
        res=3;
        return res;
    }
    if(Igual(cadena,"*")){
        if(*nnum<2){
            while(err4[i]!='\0') Terminal(err4[i++]);
            Terminal(13);
            *nnum=1;
        }
        else (*nnum)--;
        res=4;
        return res;
    }
    if(Igual(cadena,"/")){
        if(*nnum<2){
            while(err5[i]!='\0') Terminal(err5[i++]);
            Terminal(13);
            *nnum=1;
        }
        else (*nnum)--;
        res=5;
        return res;
    }
    if(Igual(cadena,"%")){
        if(*nnum<2){
            while(err6[i]!='\0') Terminal(err6[i++]);
            Terminal(13);
            *nnum=1;
        }
        else (*nnum)--;
        res=6;
        return res;
    }
    if(Igual(cadena,"=")){
        if(*nnum<2){
            while(err7[i]!='\0') Terminal(err7[i++]);
            Terminal(13);
            *nnum=1;
        }
        else (*nnum)--;
        res=7;
        return res;
    }
    if(Igual(cadena,">")){
        if(*nnum<2){
            while(err8[i]!='\0') Terminal(err8[i++]);
            Terminal(13);
            *nnum=1;
        }
        else (*nnum)--;
        res=8;
        return res;
    }
    if(Igual(cadena,"<")){
        if(*nnum<2){
            while(err9[i]!='\0') Terminal(err9[i++]);
            Terminal(13);
            *nnum=1;
        }
        else (*nnum)--;
        res=9;
        return res;
    }
    if(Igual(cadena,"r1"))       {res=10; (*nreg)++; return res;}
    if(Igual(cadena,"r2"))       {res=11; (*nreg)++; return res;}
    if(Igual(cadena,"r3"))       {res=12; (*nreg)++; return res;}
    if(Igual(cadena,"r4"))       {res=13; (*nreg)++; return res;}
    if(Igual(cadena,"r5"))       {res=14; (*nreg)++; return res;}
    if(Igual(cadena,"r6"))       {res=15; (*nreg)++; return res;}
    if(Igual(cadena,"r7"))       {res=16; (*nreg)++; return res;}
    if(Igual(cadena,"r8"))       {res=17; (*nreg)++; return res;}
    if(Igual(cadena,"if")){
        if(*nnum<1){
            while(err18[i]!='\0') Terminal(err18[i++]);
            Terminal(13);
            *nnum=0;
        }
        else (*nnum)--;
        res=18;
        (*nif)++;
        return res;
    }
    if(Igual(cadena,"else"))     {res=19; (*nelse)++; return res;}
    if(Igual(cadena,"then"))     {res=20; (*nthen)++; return res;}
    if(Igual(cadena,"do")){
        if(*nnum<2){
            while(err21[i]!='\0') Terminal(err21[i++]);
            Terminal(13);
            *nnum=0;
        }
        else (*nnum)-=2;
        res=21;
        (*ndo)++;
        return res;
    }
    if(Igual(cadena,"loop"))     {res=22; (*nloop)++; return res;}
    if(Igual(cadena,"insert"))   {res=23; (*nnum)++; return res;}
    if(Igual(cadena,"print")){
        if(*ncad>0) (*ncad)--;
        else
            if(*nnum>0) (*nnum)--;
            else{
                while(err25[i]!='\0') Terminal(err25[i++]);
                Terminal(13);
                *nnum=1;
            }
        res=25;
        return res;
    }
    if(Igual(cadena,"$")){
        if(*nreg>0) (*nreg)--;
        else{
            while(err26[i]!='\0') Terminal(err26[i++]);
            Terminal(13);
        }
        (*nnum)++;
        res=26;
        return res;
    }
    if(Igual(cadena,"!")){
        if(*nreg>0) (*nreg)--;
        else{
            while(err27[i]!='\0') Terminal(err27[i++]);
            Terminal(13);
        }
        if(*nnum<=0){
            while(err28[i]!='\0') Terminal(err28[i++]);
            Terminal(13);
        }
        res=27;
        return res;
    }
    if(Igual(cadena,"end"))      {res=28; (*nend)++; return res;}
    if(Igual(cadena,"break"))    {res=29; return res;}
    if(Igual(cadena,"i")){
        (*nnum)++;
        res=30;
        return res;
    }
    if(Igual(cadena,"j")){
        (*nnum)++;
        res=31;
        return res;
    }
    if(res==100){
        while(err01[i]!='\0') Terminal(err01[i++]); i=0;
        while(cadena[i]!='\0') Terminal(cadena[i++]); i=0;
        while(err02[i]!='\0') Terminal(err02[i++]); Terminal(13);
    }
return res;
}

/* funcion Push(num) introduce el numero en la pila */  
void Push(int num){
    int i;
    int cim=t[terminal].pila.cima;
    if(cim<KTamPila){
        t[terminal].pila.valores[cim]=num;
        cim++;
        t[terminal].pila.cima=cim;
    }
    else{
        for(i=0;i<KTamPila-1;i++){
            t[terminal].pila.valores[i]=t[terminal].pila.valores[i+1];
        }
        t[terminal].pila.valores[KTamPila-1]=num;
    }
}

/* funcion Pop() saca el primer numero de la pila */
int Pop(){
    int num=0;
    int cim=t[terminal].pila.cima;
    if(cim>0){
        cim--;
        num=t[terminal].pila.valores[cim];
        t[terminal].pila.valores[cim]=0;
        t[terminal].pila.cima=cim;
    }
return num;
}

/* funcion que borra un bloque */
void BorrarBloque(){
    int i;
    for(i=0;i<KTamBloque;i++) disco[t[terminal].numbloque-1][i]=' ';
}

/* funcion que edita un bloque */
void EditarBloque(){
    SalvarPantalla();
    marco();
    CargarBuffed();
    print_buffed(buffed[terminal]);
    cursor(X[terminal],Y[terminal]);
    PonerEstado(4);
}

/* funcion que comprueba la estructura del programa que se va a compilar */
void Comprobar(int nif,int nelse,int nthen,int ndo,int nloop,int nend){
    char err1[KTamOrden]="WARNING: Estructura IF ELSE THEN incompleta. Al parecer falta algun ELSE";
    char err2[KTamOrden]="WARNING: Estructura IF ELSE THEN incompleta. Al parecer sobra algun ELSE";
    char err3[KTamOrden]="WARNING: Estructura IF ELSE THEN incompleta. Al parecer falta algun THEN";
    char err4[KTamOrden]="WARNING: Estructura IF ELSE THEN incompleta. Al parecer sobra algun THEN";
    char err5[KTamOrden]="WARNING: Estructura DO LOOP incompleta. Al parecer falta algun LOOP";
    char err6[KTamOrden]="WARNING: Estructura DO LOOP incompleta. Al parecer sobra algun LOOP";
    char err7[KTamOrden]="WARNING: Final de Programa (end) se encuentra varias veces, con uno sobra";
    char err8[KTamOrden]="WARNING: NO se encuentra la orden Final de Programa (end)";
    int i=0;
    if(nif!=nelse){
        if(nif>nelse) {i=0; while(err1[i]!='\0') Terminal(err1[i++]); Terminal(13);}
        else          {i=0; while(err2[i]!='\0') Terminal(err2[i++]); Terminal(13);}
    }
    if(nif!=nthen){
        if(nif>nthen) {i=0; while(err3[i]!='\0') Terminal(err3[i++]); Terminal(13);}
        else          {i=0; while(err4[i]!='\0') Terminal(err4[i++]); Terminal(13);}
    }
    if(ndo!=nloop){
        if(ndo>nloop) {i=0; while(err5[i]!='\0') Terminal(err5[i++]); Terminal(13);}
        else          {i=0; while(err6[i]!='\0') Terminal(err6[i++]); Terminal(13);}
    }
    if(nend!=1){
        if(nend>1)    {i=0; while(err7[i]!='\0') Terminal(err7[i++]); Terminal(13);}
        else          {i=0; while(err8[i]!='\0') Terminal(err8[i++]); Terminal(13);}
    }       
}

/* funcion que compila un bloque pasandolo a bytecode */
void CompilarBloque(){
    int i,j,num;
    char aux,msg[KTamOrden]="PROGRAMA COMPILADO. Si han aparecido errores revise el codigo";
    char *a;
    int *n;
    int nnum=0,ncad=0,nreg=0,nif=0,nelse=0,nthen=0,ndo=0,nloop=0,nend=0;
    int *pnnum,*pncad,*pnreg,*pnif,*pnelse,*pnthen,*pndo,*pnloop,*pnend;
    pnnum=&nnum; pncad=&ncad; pnreg=&nreg; pnif=&nif; pnelse=&nelse; pnthen=&nthen; pndo=&ndo; pnloop=&nloop; pnend=&nend;
    CargarBuffed();
    IdentPalabras(buffed[terminal],t[terminal].palabras);
    i=0;
    VaciarByteCode();
    a=t[terminal].bytecode;
    while(t[terminal].palabras[i]!=NULL){
        if(EsNumero(t[terminal].palabras[i])){    /* si es numero */
            num=ConvertirANumero(t[terminal].palabras[i]);
            *a=0;
            a++;
            n=(int*)a;
            *n=num;
            a+=2;
            nnum++;
        }
        else{
            if(t[terminal].palabras[i][0]=='"'){  /* si es una cadena */
                *a=1;
                a++;
                j=1;
                while(t[terminal].palabras[i][j]!='\0'){
                    *a=t[terminal].palabras[i][j];
                    a++;
                    j++;
                }
                ncad++;
            }
            else{
                aux=BytCod(t[terminal].palabras[i],pnnum,pncad,pnreg,pnif,pnelse,pnthen,pndo,pnloop,pnend);
                if(aux!=100){
                    *a=aux;
                    a++;
                }
            }
        }
    i++;
    }
    Comprobar(nif,nelse,nthen,ndo,nloop,nend);
    i=0;
    while(msg[i]!='\0') Terminal(msg[i++]);
    Terminal(13);
}

/* funcion que ejecuta el bytecode */
void Ejecutar(){
    int a,b,c,r;
    int *num;
    char aux, cadena[KTamOrden];
    char msg[KTamOrden]="Introduce un Numero y pulsa INTRO: ";
    char err[KTamOrden]="ERROR - Numero Introducido INVALIDO (se ha cambiado por 0)";
    char div[KTamOrden]="ERROR - Se ha intentado dividir por 0. Programa Abortado";
    char fin[KTamOrden]="*************** FIN DE PROGRAMA ***************";
    switch (*t[terminal].pbytecode){
            case 0:                 /* es un numero */
                    t[terminal].pbytecode++;
                    num=(int*)t[terminal].pbytecode;
                    a=(int)*num;
                    Push(a);
                    t[terminal].pbytecode+=2;
                    break;
            case 1:                 /* es una cadena */
                    t[terminal].pbytecode++;
                    b=0;
                    while(*t[terminal].pbytecode>31){
                        cadena[b]=*t[terminal].pbytecode;
                        t[terminal].pbytecode++;
                        b++;
                    }
                    cadena[b]='\0';
                    cad=1;
                    break;
            case 2:                 /* es una suma */
                    a=Pop();
                    b=Pop();
                    c=b+a;
                    Push(c);
                    t[terminal].pbytecode++;
                    break;
            case 3:                 /* es una resta */
                    a=Pop();
                    b=Pop();
                    c=b-a;
                    Push(c);
                    t[terminal].pbytecode++;
                    break;
            case 4:                 /* es una multiplicacion */
                    a=Pop();
                    b=Pop();
                    c=b*a;
                    Push(c);
                    t[terminal].pbytecode++;
                    break;
            case 5:                 /* es una division entera */
                    a=Pop();
                    b=Pop();
                    if(a==0){
                        b=0;
                        while(div[b]!='\0') Terminal(div[b++]);
                        Terminal(13);
                        *t[terminal].pbytecode=28;
                    }
                    else{
                        c=b/a;
                        Push(c);
                        t[terminal].pbytecode++;
                    }
                    break;
            case 6:                 /* es el resto de la division entera */
                    a=Pop();
                    b=Pop();
                    c=b%a;
                    Push(c);
                    t[terminal].pbytecode++;
                    break;
            case 7:                 /* condicion igualdad */
                    a=Pop();
                    b=Pop();
                    if(a==b) c=1;
                    else     c=0;
                    Push(c);
                    t[terminal].pbytecode++;
                    break;
            case 8:                 /* condicion mayor */
                    a=Pop();
                    b=Pop();
                    if(b>a) c=1;
                    else    c=0;
                    Push(c);
                    t[terminal].pbytecode++;
                    break;
            case 9:                 /* condicion menor */
                    a=Pop();
                    b=Pop();
                    if(b<a) c=1;
                    else    c=0;
                    Push(c);
                    t[terminal].pbytecode++;
                    break;
            case 10:                /* referencia al registro r1 */
                    t[terminal].registro=1;
                    t[terminal].pbytecode++;
                    break;
            case 11:                /* referencia al registro r2 */
                    t[terminal].registro=2;
                    t[terminal].pbytecode++;
                    break;
            case 12:                /* referencia al registro r3 */
                    t[terminal].registro=3;
                    t[terminal].pbytecode++;
                    break;
            case 13:                /* referencia al registro r4 */
                    t[terminal].registro=4;
                    t[terminal].pbytecode++;
                    break;
            case 14:                /* referencia al registro r5 */
                    t[terminal].registro=5;
                    t[terminal].pbytecode++;
                    break;
            case 15:                /* referencia al registro r6 */
                    t[terminal].registro=6;
                    t[terminal].pbytecode++;
                    break;
            case 16:                /* referencia al registro r7 */
                    t[terminal].registro=7;
                    t[terminal].pbytecode++;
                    break;
            case 17:                /* referencia al registro r8 */
                    t[terminal].registro=8;
                    t[terminal].pbytecode++;
                    break;
            case 18:                /* empieza un if */
                    a=Pop();
                    if(a){                  /* si se cumple la condicion */
                        t[terminal].pbytecode++;
                    }
                    else{                   /* si NO se cumple la condicion */
                        b=0;
                        c=1;
                        t[terminal].pbytecode++;
                        while(c){
                            if(*t[terminal].pbytecode==19)
                                if(b==0) c=0;
                                else b--;
                            else
                                if(*t[terminal].pbytecode==18) b++;
                            t[terminal].pbytecode++;
                        }
                    }
                    break;
            case 19:                /* parte else de un if */
                    b=0;
                    c=1;
                    t[terminal].pbytecode++;
                    while(c){
                        if(*t[terminal].pbytecode==20)
                            if(b==0) c=0;
                            else b--;
                        else
                            if(*t[terminal].pbytecode==18) b++;
                        t[terminal].pbytecode++;
                    }
                    break;
            case 20:                /* final de un if */
                    t[terminal].pbytecode++;
                    break;
            case 21:                /* empieza un bucle */
                    a=Pop();
                    b=Pop();
                    t[terminal].pilabucle.bucle++;
                    t[terminal].pbytecode++;
                    t[terminal].pilabucle.contador[t[terminal].pilabucle.bucle-1]=a-b;
                    t[terminal].pilabucle.cuerpo[t[terminal].pilabucle.bucle-1]=t[terminal].pbytecode;
                    if(t[terminal].pilabucle.contador[t[terminal].pilabucle.bucle-1]<0){
                        a=1;
                        c=0;
                        while(a){
                            if(*t[terminal].pbytecode==22)
                                if(c==0) a=0;
                                else c--;
                            else
                                if(*t[terminal].pbytecode==21) c++;
                            t[terminal].pbytecode++;
                        }
                        t[terminal].pilabucle.bucle--;
                    }
                    break;
            case 22:                /* termina un bucle */
                    if(t[terminal].pilabucle.contador[t[terminal].pilabucle.bucle-1]>0){
                        t[terminal].pbytecode=t[terminal].pilabucle.cuerpo[t[terminal].pilabucle.bucle-1];
                        t[terminal].pilabucle.contador[t[terminal].pilabucle.bucle-1]--;
                    }
                    else{
                        t[terminal].pilabucle.bucle--;
                        t[terminal].pbytecode++;
                    }
                    break;
            case 23:                /* pedimos un numero por teclado */
                    b=0;
                    while(msg[b]!='\0') Terminal(msg[b++]);
                    *t[terminal].pbytecode=24;
                    PonerEstado(2);
                    break;
            case 24:                /* almacenamos en pila el numero introducido por teclado */
                    
                    if(EsNumero(t[terminal].cadnum)){
                        a=ConvertirANumero(t[terminal].cadnum);
                        Push(a);
                    }
                    else{
                        b=0;
                        while(err[b]!='\0') Terminal(err[b++]);
                        Terminal(13);
                        Push(0);
                    }
                    *t[terminal].pbytecode=23;
                    t[terminal].pbytecode++;
                    break;
            case 25:                /* imprimimos un numero o una cadena */
                    if(cad){
                        b=0;
                        while(cadena[b]!='\0') Terminal(cadena[b++]);
                        Terminal(13);
                        cad=0;
                        t[terminal].pbytecode++;
                    }
                    else{
                        a=Pop();
                        Push(a);
                        ConvertirACadena(a,t[terminal].cadnum);
                        b=0;
                        while(t[terminal].cadnum[b]!='\0') Terminal(t[terminal].cadnum[b++]);
                        Terminal(13);
                        t[terminal].pbytecode++;
                    }
                    break;
            case 26:                /* recuperar valor de un registro */
                    r=t[terminal].registro;
                    if(r!=0)
                        Push(t[terminal].reg[r-1]);
                    t[terminal].registro=0;
                    t[terminal].pbytecode++;
                    break;
            case 27:                /* almacena el valor en un registro */
                    a=Pop();
                    Push(a);
                    r=t[terminal].registro;
                    if(r>0)
                        t[terminal].reg[r-1]=a;
                    t[terminal].registro=0;
                    t[terminal].pbytecode++;
                    break;
            case 28:                /* final de programa */
                    b=0;
                    while(fin[b]!='\0') Terminal(fin[b++]);
                    Terminal(13);
                    PonerEstado(0);
                    break;
            case 29:                /* break para salir de un bucle */
                    t[terminal].pbytecode++;
                    t[terminal].pilabucle.bucle--;
                    a=1;
                    c=0;
                    while(a){
                        if(*t[terminal].pbytecode==22)
                            if(c==0) a=0;
                            else c--;
                        else
                            if(*t[terminal].pbytecode==21) c++;
                        t[terminal].pbytecode++;
                    }
                    break;
            case 30:                /* recuperar valor del contador del primer bucle */
                    a=t[terminal].pilabucle.contador[0];
                    Push(a);
                    t[terminal].pbytecode++;
                    break;
            case 31:                /* recuperar valor del contador del segundo bucle */
                    a=t[terminal].pilabucle.contador[1];
                    Push(a);
                    t[terminal].pbytecode++;
                    break;                        
            default: PonerEstado(0);
    }
}

/* funcion que analiza la orden del shell */
void AnalizaOrden(){
    int a=0;
    char err[KTamOrden]="ERROR: HAS INTRODUCIDO UN NUMERO DE BLOQUE INVALIDO. Hay 20 Bloques";
    IdentPalabras(t[terminal].orden.linea,t[terminal].palabras);
                                        /* si queremos editar un bloque lo cargamos en el editor */
    if((EsNumero(t[terminal].palabras[0]))&&(Igual(t[terminal].palabras[1],"ed"))){
        t[terminal].numbloque=ConvertirANumero(t[terminal].palabras[0]);
        if((t[terminal].numbloque>=1)&&(t[terminal].numbloque<=KNumBloques)){
            EditarBloque();
        }
        else{
            while(err[a]!='\0') Terminal(err[a++]);
            Terminal(13);
        }
    }
                                        /* si queremos compilar un bloque lo pasamos a bytecode */
    if((EsNumero(t[terminal].palabras[0]))&&(Igual(t[terminal].palabras[1],"cp"))){
        t[terminal].numbloque=ConvertirANumero(t[terminal].palabras[0]);
        if((t[terminal].numbloque>=1)&&(t[terminal].numbloque<=KNumBloques)){
            CompilarBloque();
        }
        else{
            while(err[a]!='\0') Terminal(err[a++]);
            Terminal(13);
        }
    }
                                        /* si queremos ejecutar el bytecode */
    if(Igual(t[terminal].palabras[0],"ej")){
        t[terminal].pbytecode=t[terminal].bytecode;
        for(a=0;a<KTamPila;a++) t[terminal].pila.valores[a]=0; t[terminal].pila.cima=0;
        for(a=0;a<KNumBucles;a++){
            t[terminal].pilabucle.contador[a]=0;
            t[terminal].pilabucle.cuerpo[a]=NULL;
        }
        t[terminal].pilabucle.bucle=0;
        t[terminal].estado=3;
    }
    
                                        /* si queremos borrar un bloque */
    if((EsNumero(t[terminal].palabras[0]))&&(Igual(t[terminal].palabras[1],"br"))){
        t[terminal].numbloque=ConvertirANumero(t[terminal].palabras[0]);
        if((t[terminal].numbloque>=1)&&(t[terminal].numbloque<=KNumBloques)){
            BorrarBloque();
        }
        else{
            while(err[a]!='\0') Terminal(err[a++]);
            Terminal(13);
        }
    }
}

/* funcion que trata el caracter recibido del buffer */
void TratarCaracter(char caracter){
    int i;
    switch(caracter){
        case 18:                       /* retroceso */
                if(t[terminal].orden.p>0){
                    t[terminal].orden.p--;
                    t[terminal].orden.linea[t[terminal].orden.p]=' ';
                }
                Terminal(caracter);
                break;
        case 13:                      /* intro (final de la orden) */
                t[terminal].orden.linea[t[terminal].orden.p]='\0';
                Terminal(caracter);
                AnalizaOrden();
                for (i=0;i<KTamOrden;i++) t[terminal].orden.linea[i]=' '; t[terminal].orden.p=0;
                break;
        default:
                t[terminal].orden.linea[t[terminal].orden.p]=caracter;
                t[terminal].orden.p++;
                Terminal(caracter);
                break;
    }
}



/*********** PROGRAMA PRINCIPAL *************/
int main(){
/* Variables */
char car,aux;
int i=0,neg=2;
char *b;

/* guarda el vector de interrupciones del teclado inicial */
    int *t=(int *)MK_FP(0,36);
    int desT,segT;
    desT=*t;
    segT=*(t+1);

/* cambia el vector de interrupciones del teclado */
    asm cli;
    *t=FP_OFF(rutinaTeclado);
    *(t+1)=FP_SEG(rutinaTeclado);
    asm sti;
    
    BorrarPantalla();
    DesactivarCursorHW();
    InicTabla();
    InicEstructuras();
    InicializarDisco();
    CursorOn();
    terminal=0;
    while(teclaPulsada!=1){ /* mientras no se pulse la tecla ESC */
        switch(VerEstado()){
            case 0: 
                    break;                            /* shell esperando tecla */
            case 1:                                   /* shell ejecutandose */
                    while(TamBuffTec()>0){            /* paso los caracteres del buffer a la orden del shell y al terminal*/
                        car=ExtraerDelBuffer();
                        TratarCaracter(car);
                    }
                    if(VerEstado()==1) PonerEstado(0);
                    break;
            case 2: 
                    break;
            case 3: 
                    if(TamBuffTec()>0){                /* si hay caracteres en el buffer */
                        while(TamBuffTec()>0){         /* recojo los caracteres del buffer */
                            car=ExtraerDelBuffer();
                            if(car==13){               /* si hemos introducido un intro (numero compelto) */
                                if(i==0){CadNum(i,'0'); i++;}
                                CadNum(i,'\0');
                                i=0;
                                Terminal(car);
                                Ejecutar();      
                            }
                            else{                      
                                if(car==18){           /* si hemos introducido BACK */
                                    if(i>0){
                                        i--;
                                        CadNum(i,' ');
                                        Terminal(car);
                                    }
                                    PonerEstado(2);
                                }
                                else{
                                    if(CadNumNeg()) neg=1;
                                    if(i<KTamCadNum-neg){    /* si hemos introducido otro caracter */
                                        CadNum(i,car);
                                        i++;
                                        Terminal(car);
                                    }
                                    neg=2;
                                    PonerEstado(2);
                                }
                            }
                        }
                    }
                    else{                             /* si no hay caracteres en el buffer sigo ejecutando */
                        Ejecutar();
                    }
                    break;
            case 4: 
                    break;
            case 5:
                    while(TamBuffTec()>0){            /* paso los caracteres del buffer al editor*/
                        car=ExtraerDelBuffer();
                        ed(car,buffed[terminal]);
                        }
                        if(VerEstado()==5) PonerEstado(4);
                    break;
        }
    }
    GuardarEnDisco();
    
/* restaura el vector de interrupciones del teclado y lo deja como estaba inicialmente */
    asm cli;
    *t=desT;
    *(t+1)=segT;
    asm sti;

return(0);
}


