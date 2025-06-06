# Mini intérprete en español

Este proyecto contiene un pequeño intérprete del lenguaje **Gama** con palabras reservadas en español. Ahora el código se encuentra dividido en módulos (`lexer`, `parser`, `symtab` y `main`).

El intérprete soporta declaraciones de variables, lectura/escritura, condicionales, bucles y una construcción `Switch` básica.

## Compilación

En Linux o Windows con GCC:

```bash
gcc -Wall src/*.c -o gama
```

## Uso

El ejecutable acepta el archivo fuente como argumento. Opcionalmente puede generarse un archivo `.obj` con la lista de tokens:

```bash
./gama programa.cpp [tokens.obj]
```

El archivo `input.txt` proporciona un ejemplo de programa.

## Licencia

Este proyecto se distribuye bajo la licencia MIT. Consulte el archivo `LICENSE`.
