# Silver (1999) Remaster - Español Castellano & Android Touch Controls

![Silver Cover](https://upload.wikimedia.org/wikipedia/en/thumb/e/e0/Silver_Coverart.png/250px-Silver_Coverart.png)

Adaptación y remasterización completa del clásico RPG de acción **Silver (1999)** (desarrollado por Spiral House e Infogrames) con **traducción oficial íntegra al castellano** y **capa de control táctil y gestual nativa** para jugar en dispositivos móviles Android (vía Winlator) o en PC sin necesidad de teclado ni mandos virtuales invasivos.

---

## 🌟 Características de esta Edición

* **Doblaje y Voces Completas en Español:**  
  Todos los 1.256 archivos de audio y diálogos de personajes en español oficial de la edición española de 1999, convertidos y sincronizados en alta calidad (`.wav` y `.ogg`).
* **Textos y Subtítulos en Castellano:**  
  Los 118 scripts y diálogos `.eng` traducidos con sus tablas de codificación de caracteres.
* **Cinemática Introductoria Doblada:**  
  `intro.avi` restaurado con el audio y doblaje en castellano de la versión física original.
* **Capa Táctil Nativa y Reconocimiento de Gestos (`SDL2.dll`):**  
  Interceptor de eventos SDL2 de baja latencia que transforma deslizamientos en pantalla (*swipes*) en ataques de espada fluidos (estocadas, tajos y bloqueo con escudo) y pulsaciones largas en la apertura automática del menú radial.
* **Lanzador Android Autónomo (`.apk`):**  
  Aplicación instalable para Android con el logo oficial del juego en alta resolución, que arranca a pantalla completa sin escritorios ni configuraciones manuales.

---

## 🎮 Controles Táctiles y Gestos (Android)

| Gesto en Pantalla | Acción en el Juego | Descripción |
| :--- | :--- | :--- |
| **Tap Rápido** | **Caminar / Interactuar** | Toca el suelo para moverte, o toca NPCs/cofres para hablar o abrirlos. |
| **Doble Tap** | **Correr** | David echa a correr hacia la dirección señalada. |
| **Mantener Pulsado (>0.4s)** | **Menú Radial / Anillo** | Abre instantáneamente el anillo de magias, inventario y cambio de personajes. |
| **Deslizar hacia Arriba (Swipe ↑)** | **Estocada (Thrust)** | Ataque frontal directo de gran alcance. |
| **Deslizar a la Izquierda (Swipe ←)** | **Tajo hacia la Izquierda** | Barrido de espada para atacar a múltiples enemigos a la izquierda. |
| **Deslizar a la Derecha (Swipe →)** | **Tajo hacia la Derecha** | Barrido de espada para atacar a la derecha. |
| **Deslizar hacia Abajo (Swipe ↓)** | **Bloqueo con Escudo** | Alza el escudo para protegerse de proyectiles y golpes directos. |

*La sensibilidad de los gestos se puede personalizar en `silver_touch.ini`.*

---

## 📱 Cómo Instalar y Jugar en Android

1. **Instalar la App:**  
   Descarga e instala [Silver_Android_Español.apk](Silver_Android_Espa%C3%B1ol.apk) en tu teléfono Android.
2. **Copiar la carpeta del juego:**  
   Copia la carpeta del juego `Silver` a la memoria interna de tu dispositivo (ruta recomendada: `/storage/emulated/0/Download/Silver`).
3. **Configuración única en Winlator:**  
   * Abre tu contenedor en Winlator.
   * En el menú lateral izquierdo, configura el modo de ratón en **Absolute / Direct Touch**.
   * Oculta los controles táctiles virtuales en pantalla (*Show Controls -> OFF* u Opacidad 0%), ya que nuestra capa táctil maneja todo de forma invisible.
   * Haz clic derecho sobre `silver.exe` y selecciona **Create Shortcut**.
4. **¡Listo!**  
   Toca el icono de **Silver** en la pantalla de inicio de tu teléfono para jugar a pantalla completa.

---

## 📁 Estructura del Repositorio

* `Silver_Android_Español.apk` — Lanzador Android con icono oficial y orientación horizontal forzada.
* `SDL2.dll` — Proxy DLL compilado para x86 con el interceptor de gestos táctiles.
* `silver_touch.ini` — Archivo de configuración de umbrales y sensibilidad táctil.
* `touch_hook.c` — Código fuente en C de la capa de gestos y síntesis de eventos.
* `sdl2_proxy.def` — Tabla de reenvío de exportaciones PE para SDL2.
* `INSTRUCCIONES_SILVER_ANDROID.txt` — Guía completa de uso y solución de dudas.

---

## ⚖ Créditos y Licencias

* *Silver* © 1999 Spiral House Ltd. / Infogrames.
* Remasterización de PC © 2017 THQ Nordic.
* Doblaje y traducción al español extraídos de los discos originales de 1999 (Infogrames España).
* Capa táctil y empaquetado para Android por [cggvallejo](https://github.com/cggvallejo).
