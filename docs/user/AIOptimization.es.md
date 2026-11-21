# Asistente de ajustes con IA

Kaichii puede pedir a OpenAI o Anthropic una propuesta de ajustes para tu equipo. Necesitas una
clave de API propia del proveedor elegido; el servicio puede cobrarte por cada consulta. La función
es opcional: los perfiles de rendimiento y **Optimizar para este juego** siguen funcionando sin IA.

## Cómo usarlo

- **Para todos los juegos:** abre *Emulación > Pedir a la IA que ajuste este equipo*. Cierra el juego
  antes. La propuesta modifica los ajustes globales y se prueba tras reiniciar Kaichii.
- **Para un juego:** haz clic derecho en él y elige *Pedir a la IA que ajuste este juego*. También
  aparece en *Emulación* mientras juegas. Puedes consultar con el juego cerrado. Si está abierto,
  Kaichii añade a la consulta los FPS y el uso de memoria recientes, cuando hay datos. Para aplicar
  los cambios, te preguntará si quieres detener el juego; se prueban al iniciarlo de nuevo.

Elige el proveedor y el modelo, escribe tu clave, describe el problema y pulsa **Pedir una
propuesta**. La lista de modelos muestra los principales de cada proveedor, primero el recomendado;
puedes escribir otro nombre o pulsar **Cargar del proveedor** para ver los modelos que tu clave puede
usar. Kaichii recuerda el último proveedor y modelo que usaste.

Para no escribir la clave cada vez, marca **Recordar esta clave en el almacén de credenciales del
sistema**. Cuando una consulta sale bien, la guarda el sistema operativo: el Llavero en macOS, el
Administrador de credenciales en Windows y el Secret Service (GNOME Keyring, KWallet) en Linux, a
través de libsecret. Nunca se escribe en los archivos de configuración de Kaichii, y Kaichii solo la
envía a tu proveedor. **Olvidar la clave guardada** la borra, y también una consulta correcta con la
casilla desmarcada. Si no hay almacén de credenciales (Linux sin libsecret), la casilla aparece
desactivada y la clave solo permanece en memoria mientras la ventana está abierta.

La propuesta enseña cada cambio, su motivo y el valor anterior. Desmarca lo que no quieras aplicar.
Kaichii solo acepta opciones y valores de su catálogo, y guarda los ajustes anteriores. Puedes volver
a abrir el asistente y pulsar **Deshacer el último cambio de IA**. Si después modificas manualmente
alguno de esos ajustes, el asistente no lo sobrescribe al deshacer.

Para seguir afinando, prueba el juego y vuelve a describir el resultado. Una escena distinta o la
primera compilación de shaders puede alterar los FPS, así que compara en el mismo lugar. La IA puede
explicar síntomas y elegir ajustes; no repara fallos de emulación. Si faltan texturas incluso con
ajustes apropiados, consulta [Solución de problemas](Troubleshooting.es.md).

## Datos y alcance

La consulta incluye tu descripción, el nombre e identificador del juego, sistema operativo, GPU,
VRAM, hilos de CPU, RAM, ajustes pertinentes, el último cambio hecho por el asistente y, si el juego
está en marcha, las métricas recientes disponibles. No se
envían juegos, partidas, claves de la consola, capturas ni el registro completo. Con el modo
desarrollador activado (en la configuración), la ventana muestra además el diagnóstico exacto, en
**Datos enviados con esta consulta**, antes de enviarlo. El resultado se valida localmente; una respuesta inválida
no cambia la configuración.
