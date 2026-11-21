# Lisímetro electrónico de pesada – Prototipo V1

Lisímetro electrónico de pesada para estimar la **evapotranspiración (ET)** a partir de la variación de masa de un volumen de suelo con cobertura vegetal. Proyecto del curso **Instrumentación Industrial** – Escuela Profesional de Ingeniería Electrónica y Telecomunicaciones, **Universidad Nacional de Piura (UNP)**, 2026.

> **Estado:** Prototipo V1, en verificación funcional preliminar (Entregable 2). Este no es un instrumento calibrado: los resultados son preliminares y no tienen trazabilidad metrológica demostrada.

---

## Tabla de contenido

- [Principio de funcionamiento](#principio-de-funcionamiento)
- [Cadena de medición](#cadena-de-medición)
- [Hardware](#hardware)
- [Software y entorno](#software-y-entorno)
- [Estructura del repositorio](#estructura-del-repositorio)
- [Instalación y compilación](#instalación-y-compilación)
- [Calibración del prototipo](#calibración-del-prototipo)
- [Formato de datos](#formato-de-datos)
- [Limitaciones de la versión V1](#limitaciones-de-la-versión-v1)
- [Equipo](#equipo)
- [Referencias](#referencias)
- [Licencia](#licencia)

---

## Principio de funcionamiento

El lisímetro de pesada mide la masa total del recipiente con suelo y vegetación. Si se corrigen los aportes de riego, la lluvia y el drenaje, la disminución de masa en un intervalo de tiempo corresponde al agua perdida por evapotranspiración:

```
ET [mm] = Δm [kg] / (ρw [kg/m³] · A [m²]) · 1000
```

donde `Δm` es la pérdida de masa, `ρw ≈ 1000 kg/m³` es la densidad del agua y `A` es el área superficial del lisímetro.
Equivalencia útil: **1 kg de agua perdida sobre 1 m² = 1 mm de ET**.

- Área superficial del lisímetro: **[COMPLETAR] m²**
- Masa total aproximada (recipiente + suelo): **[COMPLETAR] kg**
- Resolución de masa objetivo: **[COMPLETAR] g**, que equivale a **[COMPLETAR] mm** de ET

## Cadena de medición

```
Masa (mensurando)
   │
   ▼
Celda(s) de carga  ──►  Excitación  ──►  AFE / ADC  ──►  Microcontrolador  ──►  Visualización / registro
[COMPLETAR modelo]      [COMPLETAR V]    [COMPLETAR]     [COMPLETAR placa]      [LCD / SD / serial / WiFi]
```

## Hardware

| Bloque | Componente | Observación |
|---|---|---|
| Sensor | [COMPLETAR: modelo de la celda de carga, capacidad, cantidad] | [p. ej., configuración en puente completo] |
| Acondicionamiento / ADC | [COMPLETAR: p. ej., HX711 / ADS1232 / INA + ADC] | Ganancia: [COMPLETAR], resolución: [COMPLETAR] bits |
| Microcontrolador | [COMPLETAR: Arduino / ESP32 / otro] | |
| Visualización | [COMPLETAR] | |
| Registro | [COMPLETAR: microSD / serial / nube] | |
| Reloj | [COMPLETAR: RTC, si aplica] | |
| Alimentación | [COMPLETAR] | |

El esquemático completo está en [`hardware/`](hardware/) y la lista de materiales en [`docs/BOM.xlsx`](docs/).

## Software y entorno

| Herramienta / librería | Versión | Uso |
|---|---|---|
| [COMPLETAR: Arduino IDE / PlatformIO] | [COMPLETAR] | Entorno de desarrollo |
| [COMPLETAR: core de la placa] | [COMPLETAR] | Soporte del microcontrolador |
| [COMPLETAR: librería del ADC] | [COMPLETAR] | Lectura de la celda de carga |
| [COMPLETAR] | [COMPLETAR] | [COMPLETAR] |

Las librerías de terceros **no se incluyen** en este repositorio. Para compilar, instálelas desde el gestor de librerías del entorno, en las versiones indicadas.

## Estructura del repositorio

```
lisimetro-pesada-v1/
├── firmware/          # Código fuente del microcontrolador
├── hardware/          # Esquemático (archivos editables + PDF)
├── datos/             # Datos brutos de ensayos (CSV/XLSX)
├── analisis/          # Scripts u hojas de cálculo para gráficas y estadística
├── docs/              # Informe, BOM, datasheets, fotografías
└── README.md
```

## Instalación y compilación

1. Clonar el repositorio:
   ```bash
   git clone https://github.com/[USUARIO]/lisimetro-pesada-v1.git
   ```
2. Abrir `firmware/[COMPLETAR].ino` en [COMPLETAR: Arduino IDE / PlatformIO].
3. Instalar las librerías de la tabla [Software y entorno](#software-y-entorno).
4. Seleccionar la placa **[COMPLETAR]** y el puerto correspondiente.
5. Compilar y cargar el firmware.
6. Abrir el monitor serie a **[COMPLETAR] baudios**.

## Calibración del prototipo

El firmware convierte el código del ADC a masa con un modelo lineal:

```
m [kg] = (código − offset) / factor_escala
```

Procedimiento de ajuste de V1:

1. **Tara (cero):** con la plataforma vacía, o con el recipiente vacío, registrar el `offset`.
2. **Span:** colocar una masa de referencia conocida de **[COMPLETAR] kg** y calcular el `factor_escala`.
3. Guardar ambos valores en [COMPLETAR: código / EEPROM].
4. Verificar con al menos 5 masas distribuidas en el rango de operación, tomando 3 lecturas por punto.

Referencia utilizada: **[COMPLETAR: balanza o pesas, modelo, resolución, certificado si existe]**.

> Este procedimiento corresponde a una **verificación funcional y un ajuste preliminar**. No es una calibración formal.

## Formato de datos

Los archivos de [`datos/`](datos/) usan el siguiente formato (ajustar a las columnas reales):

| Columna | Unidad | Descripción |
|---|---|---|
| `fecha_hora` | AAAA-MM-DD hh:mm:ss | Marca de tiempo |
| `codigo_adc` | cuentas | Lectura cruda del ADC |
| `masa` | kg | Masa calculada |
| `masa_ref` | kg | Valor de la referencia (en ensayos) |
| `temp_amb` | °C | Temperatura ambiente, si se registra |

- Frecuencia de actualización efectiva: **[COMPLETAR] Hz / s**
- Tratamiento de lecturas inválidas: **[COMPLETAR: p. ej., descarte por timeout del ADC, valores fuera de rango]**

## Limitaciones de la versión V1

- Prototipo en protoboard o montaje provisional. Todavía no hay PCB ni carcasa definitiva.
- No se ha completado la evaluación de la deriva térmica ni del creep de la celda de carga.
- Falta la incertidumbre expandida final, que queda pendiente para la etapa de validación.
- [COMPLETAR: otras limitaciones observadas]

Las correcciones previstas se detallan en el plan V2 del informe técnico.

## Equipo

| Integrante | Rol |
|---|---|
| [COMPLETAR] | [p. ej., Hardware y AFE] |
| [COMPLETAR] | [p. ej., Firmware] |
| [COMPLETAR] | [p. ej., Pruebas y análisis de datos] |

**Docente:** Mg. Ing. Yonatan Aguirre – Instrumentación Industrial, UNP, 2026.

## Referencias

- Allen, R. G., Pereira, L. S., Raes, D., & Smith, M. (1998). *Crop evapotranspiration: Guidelines for computing crop water requirements* (FAO Irrigation and Drainage Paper No. 56). FAO.
- [COMPLETAR: datasheet de la celda de carga]
- [COMPLETAR: datasheet del ADC / módulo de acondicionamiento]

## Licencia

[COMPLETAR: p. ej., MIT para el código y CC BY 4.0 para la documentación]
