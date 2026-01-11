# F4MP - Fallout 4 Multiplayer Project 🚀

<p align="center">
  <img src="/logo.png" alt="F4MP Logo" width="300">
  <br>
  <i>Reconstruyendo la Commonwealth, un paquete de datos a la vez.</i>
</p>

![Game](https://img.shields.io/badge/Game-Fallout%204-brightgreen?style=for-the-badge&logo=bethesda)
![Status](https://img.shields.io/badge/Status-Research_&_Development-orange?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-PC-blue?style=for-the-badge)

---

## 📝 Descripción del Proyecto

**F4MP** es un ambicioso proyecto de código abierto que busca implementar una infraestructura multijugador robusta para **Fallout 4**. A diferencia de otros intentos, F4MP se centra en la creación de un sistema de sincronización basado en un servidor maestro que permita la persistencia de datos, el combate cooperativo y la construcción de asentamientos compartidos.

Este proyecto es de carácter **educativo y sin ánimo de lucro**, desarrollado por y para la comunidad de entusiastas de la saga.

---

## 🔬 Fase Actual: Investigación y Análisis (R&D)

Actualmente, el repositorio **no contiene binarios ejecutables**. Nos encontramos en una fase de ingeniería inversa profunda para asegurar que la base del mod sea estable antes de cualquier lanzamiento público.

### Objetivos de Investigación Crítica:
* **Sincronización de Transformaciones:** Mapeo de vectores de posición y rotación de entidades en el Creation Engine.
* **Hooking de Memoria:** Implementación de interceptores para acciones de combate (VATS, disparo, daño recibido).
* **World State Sync:** Análisis de la persistencia de objetos soltados y cambios en el entorno (Cells).
* **Protocolo de Red:** Desarrollo de una capa de transporte híbrida UDP/TCP para minimizar la latencia en el desierto capital.

---

## ⚙️ Arquitectura del Sistema

El ecosistema F4MP se compone de tres pilares tecnológicos:

1.  **F4MP Client Core:** Un inyector desarrollado en C++ que actúa como puente entre el motor del juego y nuestra red.
2.  **Master Server:** Backend escalable encargado de la validación de usuarios, gestión de instancias y retransmisión de estados.
3.  **Terminal de Control Web:** Interfaz de usuario para la gestión de residentes y monitorización del sistema.



---

## 🌐 Seguimiento y Progreso

Para evitar la fragmentación de la información, el progreso detallado de cada fase se publica exclusivamente en nuestra terminal oficial. Allí podrás ver el estado de los módulos de investigación y los hitos alcanzados.

👉 **[CONSULTAR ROADMAP OFICIAL EN LA WEB](https://f4mp.joustech.space/roadmap.php)**

---

## 🤝 Cómo contribuir

Si tienes conocimientos en **ingeniería inversa, C++, Assembly (x64)** o **protocolos de red**, tu ayuda es bienvenida. 

1.  Haz un **Fork** del proyecto.
2.  Crea una rama para tu investigación (`git checkout -b feature/investigacion-x`).
3.  Abre un **Pull Request** detallando tus hallazgos en la memoria del juego.

---

## ⚖️ Aviso Legal (Disclaimer)

F4MP es un proyecto independiente y no está afiliado a Bethesda Softworks ni ZeniMax Media. El uso de este software es bajo tu propio riesgo y requiere una copia legal de Fallout 4. Todos los nombres y marcas registradas pertenecen a sus respectivos dueños.

---
<p align="center">
  Propiedad de ROBCO INDUSTRIES (Jous99) // 2026
</p>
