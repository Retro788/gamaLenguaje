# Mini intérprete en español

Este proyecto implementa un intérprete muy sencillo que reconoce un subconjunto de C en español. 
Permite declarar variables enteras, leer y escribir valores, asignar expresiones y controlar el flujo con `Si`/`Sino` y `Mientras`.

## Compilación

En Windows con GCC:

```bash
gcc -Wall analyzer.c -o analyzer.exe
```

En Linux:

```bash
gcc -Wall analyzer.c -o analyzer
```

## Uso

El intérprete lee el programa de la entrada estándar. Se puede ejecutar con un archivo de ejemplo:

```bash
./analyzer < input.txt
```

El archivo `input.txt` contiene un programa que demuestra declaraciones, asignaciones y bucles.

## Licencia

Este proyecto se distribuye bajo los términos de la licencia MIT. Consulte el archivo `LICENSE`.
