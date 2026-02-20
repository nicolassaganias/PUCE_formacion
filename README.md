# Living Lab PUCE-Ibarra: Sensores Ambientales Open Source

Curso de fortalecimiento de capacidades en innovación docente y living labs para la resiliencia territorial frente al cambio climático. Pontificia Universidad Católica del Ecuador, Sede Ibarra - Universidad de Girona, marzo 2026.

## Sobre el curso

Este repositorio contiene los materiales de la capacitación en montaje y calibración de sensores ambientales low-cost. El enfoque está puesto en herramientas abiertas (Arduino, hardware libre, software open source) para el monitoreo participativo de territorios vivos.

Los participantes desarrollan prototipos funcionales de dos líneas:

- **Kit Ciencia:** sensores para calidad de agua (pH, TDS, turbidez, temperatura, oxígeno disuelto, conductividad)
- **Kit Arquitectura:** sensores para condiciones ambientales en espacios interiores (iluminación, temperatura, humedad, ruido, gases, ocupación)

## Estructura del repositorio

| Carpeta | Contenido |
|---------|-----------|
| `firmware/` | Códigos Arduino organizados por sensor y funcionalidad |
| `downloads/` | Instructivos de instalación: Arduino IDE, driver CH340, librerías necesarias |
| `documentacion/` | Diagramas de conexión, esquemas eléctricos y guías de montaje |
| `diseño 3d/` | Archivos STL y fuentes para impresión de cajas y soportes de sensores |

## Hardware utilizado

- Arduino UNO / Nano
- Sensores: DS18B20, DHT22, BME280, pH, TDS, turbidez, módulos de sonido y luminosidad
- Almacenamiento: módulos SD
- Comunicación: I2C, SPI, OneWire

## Licencia

Los códigos y diseños se comparten bajo licencias open source para fomentar la replicación en otros contextos educativos y comunitarios.

---

**Instructor:** Mgtr. Nicolás Saganias  
**Coordinación:** Mgtr. Guillermo Guzmán, Mgs. Moraima Mera  
**Proyecto:** Innovación docente y Living Lab universitario - Convocatoria de cooperación UdG
