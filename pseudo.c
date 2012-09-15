/*
Este es el pseudocodigo del programa, con los
aspectos fundamentales pero claro esta sin
descender a los detalles particulares, que cada
cual disenyara  de la forma que crea mejor. Sin
embargo, se supone que existe un array de
cuatro estructuras, conteniendo cada una todo
lo necesario para la ejecucion del proceso:
el array de bytecode, la pila aritmetica, la
pila para bucles, la pantalla virtual... en fin,
todo lo necesario.

En el pseudocodigo usare la variable N para 
indicar el proceso en ejecucion, y T para indicar
el proceso que es en un momento dado el propietario
del teclado y pantalla fisicos.
*/


/*
interrupcion de teclado
*/
void interrupt SIT()
{
    - tomar el codigo depositado por el hardware
      en el puerto 0x60
    - transformarlo a codigo ascii
    - si es el codigo que indica el cambio de
      terminal:
          a) salvar pantalla fisica a pantalla virtual
           de T
        b) pasar al proceso siguiente
        c) copiar su pantalla virtual en la pantalla
           fisica y actualizar T
        d) salir
        
    - consultar el byte de estado de T, e(T)

        a) si e(T)=="shell esperando tecla" 
            depositar codigo en el buffer y
            cambiar e(T) a "shell ejecutandose"
        b) si e(T)=="programa esperando tecla"
            depositar codigo en el buffer y
            cambiar e(T) a "programa ejecutandose"
        c) si e(T)=="editor esperando tecla"
            depositar codigo en el buffer y cambiar
            estado a "editor ejecutandose"
}

main()
{
    - iniciar estructuras de datos
    - redireccionar vector de interrupcion de teclado
      a SIT(), limpiar pantalla fisica y pantallas
      virtuales y todas las operaciones previas que
      sean necesarias
    - N=0
    - while (1){

        - consultar estado de N
        - si e(N)="shell esperando tecla", pasar
        - si e(N)="programa esperando tecla", pasar
        - si e(N)="editor esperando tecla", pasar
        - si e(N)="shell ejecutandose"
            a) tomar teclas del buffer de teclado
            b) si una tecla es alfanumerica, pasarla 
               al buffer donde se compone la orden, y 
               enviar copia al terminal. Volver a
               "shell esperando tecla"
            c) si es borrar atras, actualizar buffer
               donde se compone la orden, y enviar 
               "borrar atras al terminal". Volver a
               "shell esperando tecla"
            d) si es INTRO, indicando fin de linea
                d.1) analizar la linea
                    - si tiene la forma "x ed",
                      donde x es un numero, editar
                      el bloque x. Para ello, cargar
                      el bloque en el buffer y poner
                      al proceso en estado "editor
                      esperando tecla"
                    - si tiene la forma "x cp" compilar
                      el bloque x a bytecode
                    - si tiene la forma "ej" pasar a
                      "ejecutando programa"
                    - si es otra cosa, emitir un 
                      mensaje de error y volver al estado
                      "shell esperando tecla"


        - si e(N)=="programa ejecutandose"
            ejecutar en un bucle un numero prefijado de 
            bytecodes. Si alguno de los bytecodes implica 
            que el programa espera una tecla del usuario, 
            cambiar e(N) a "programa esperando tecla" y
            salir del bucle. Si se encuentra el bytecode
            de fin de programa, emitir un mensaje por el
            terminal y poner e(N)="shell esperando tecla".
            Si se produce un error (division por cero u
            otro) emitir un mensaje de error y poner
            e(N)="shell esperando tecla"

        - si e(N)=="editor ejecutandose"
            pasarle la tecla a ed(). Si es el codigo de
            salida, cambiar a "shell esperando tecla",
            en caso contrario, volver a "editor esperando
            tecla"
        
        incrementar N
        
}
