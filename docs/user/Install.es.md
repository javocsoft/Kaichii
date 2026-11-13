# Instalar Kaichii

Las versiones están en la [página de releases](https://github.com/javocsoft/Kaichii/releases). Cada
una viene en versión normal y **PGO**: el mismo programa, compilado con perfiles grabados mientras se
juega. Elige la PGO cuando exista para tu sistema.

## Linux

Descarga `kaichii-linux-x86_64-pgo.AppImage` (o `kaichii-linux-aarch64-pgo.AppImage` en ARM), hazlo
ejecutable y ábrelo:

```sh
chmod +x kaichii-linux-x86_64-pgo.AppImage
./kaichii-linux-x86_64-pgo.AppImage
```

El AppImage lleva todo lo que necesita. Funciona en sistemas con glibc 2.39 o más reciente: Ubuntu
24.04, Fedora 40, Debian 13, Arch y las distribuciones basadas en ellos.

- **Dice que necesita FUSE.** Instala el paquete de FUSE 2 de tu distribución (`libfuse2t64` en
  Ubuntu 24.04, `fuse2` en Arch), o ábrelo con `APPIMAGE_EXTRACT_AND_RUN=1 ./kaichii-…AppImage`.
- **El menú de aplicaciones y el icono en la barra de tareas.** Los escritorios muestran el icono
  de una aplicación a partir de su entrada en el menú de aplicaciones, y un AppImage no la instala
  por sí solo. La primera vez que arranca, Kaichii ofrece añadirse ahí; si aceptas, la barra de
  tareas muestra su icono. **Añadir Kaichii al menú de aplicaciones**, en *Configurar > Interfaz*,
  lo cambia más tarde.
- **Prefieres usar las bibliotecas de tu sistema.** El `.tar.gz` trae los mismos programas sin sus
  bibliotecas; necesitas Qt 6 y las bibliotecas habituales instaladas.

## Windows

Descarga `kaichii-windows-x86_64-pgo.zip`, extrae **la carpeta entera** donde quieras y abre
`kaichii.exe`.

- **"Windows protegió su PC".** SmartScreen aún no conoce el programa: elige *Más información* y
  *Ejecutar de todas formas*.
- **Falta `VCRUNTIME140.dll` o `MSVCP140.dll`.** Instala el
  [Visual C++ Redistributable](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist) de
  Microsoft para x64.

## macOS

Descarga `kaichii-macos-arm64.zip` (Apple Silicon, macOS 15 o más reciente) y descomprímelo. La app
no está notarizada, así que la primera vez haz clic derecho sobre ella y elige *Abrir*, o ejecuta
`xattr -dr com.apple.quarantine kaichii.app`. El soporte de macOS es experimental.

## Actualizaciones

Kaichii busca una versión nueva cada vez que arranca y te ofrece instalarla. El AppImage y la carpeta
de Windows se actualizan solos; tus partidas y ajustes se quedan. Puedes desactivarlo con **Buscar
actualizaciones** en *Configurar > Interfaz* (o escribiendo su nombre en el buscador de ajustes).

## Dónde se guardan tus datos

Kaichii guarda partidas, claves, firmware, ajustes y cachés de shaders en una carpeta llamada `eden`
(el nombre de un proyecto anterior, que se mantiene para no perder nada):

| Sistema | Carpeta |
|---|---|
| Linux | `~/.local/share/eden` (datos), `~/.config/eden` (ajustes), `~/.cache/eden` (cachés) |
| Windows | `%APPDATA%\eden` |

*Archivo > Abrir carpetas de Kaichii* las abre. En Windows, una carpeta llamada `user` junto a
`kaichii.exe` hace Kaichii portátil: todo se guarda ahí.

### Si vienes de otro emulador

Lichii y Eden usan la misma carpeta `eden`, así que Kaichii encuentra tus partidas, ajustes y cachés
tal como están. Desde yuzu y los emuladores basados en él, Kaichii ofrece importar sus datos la
primera vez que arranca. Una versión que todavía se llama Lichii no se actualiza sola a Kaichii:
descarga Kaichii una vez a mano.

Siguiente: [Primer arranque](FirstLaunch.es.md).
