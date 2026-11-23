// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/game_companion_catalog.h"

#include <array>

#include <QChar>
#include <QStringList>

namespace GameCompanion {
namespace {

constexpr std::string_view Classic =
    "https://zelda.fandom.com/wiki/The_Legend_of_Zelda:_Ocarina_of_Time";
constexpr std::string_view Remake =
    "https://zelda.fandom.com/wiki/The_Legend_of_Zelda:_Ocarina_of_Time_(Switch_2)";
constexpr std::string_view Nintendo =
    "https://www.nintendo.com/us/whatsnew/its-time-experience-the-legend-of-zelda-ocarina-of-time-reborn-exclusively-for-nintendo-switch-2/";
constexpr std::string_view Timeline =
    "https://www.nintendo.com/es-es/Campanias-publicitarias/40-aniversario-de-The-Legend-of-Zelda/40-aniversario-de-The-Legend-of-Zelda-Cronologia-3214722.html";
constexpr std::string_view Characters = "https://zelda.mtwildwood.net/en-gb/characters/";

// Short, original descriptions. The original game's wiki supplies background; the remake page
// supplies the edition-specific differences. Empty detail means there is no extra story to reveal.
constexpr std::array Entries{
    Entry{"kokiri_forest", Kind::Place, {"Kokiri Forest", "Bosque Kokiri"},
          {"The forest where Link's adventure begins and the Kokiri live.",
           "El bosque donde comienza la aventura de Link y viven los Kokiri."},
          {"The Great Deku Tree watches over the Kokiri, and Link's house is one of the village's landmarks. The paths lead toward the Lost Woods and the first dungeon inside the tree. The forest introduces the contrast between Link's sheltered childhood and the wider kingdom waiting beyond its exit.",
           "El Gran Árbol Deku cuida de los Kokiri y la casa de Link es uno de los lugares reconocibles de la aldea. Sus caminos llevan al Bosque Perdido y a la primera mazmorra, dentro del árbol. El bosque presenta el contraste entre la infancia protegida de Link y el reino que espera tras la salida."},
          Classic, false},
    Entry{"hyrule_field", Kind::Place, {"Hyrule Field", "Pradera de Hyrule"},
          {"The open land connecting many of Hyrule's settlements.",
           "La llanura que comunica muchos de los asentamientos de Hyrule."},
          {"Its roads lead to the castle, Kakariko, the forest and Lon Lon Ranch. Time of day affects what Link meets here: the field feels open and calm by day, but night brings different hazards. Epona later changes how quickly the player can cross it.",
           "Sus caminos llevan al castillo, Kakariko, el bosque y el Rancho Lon Lon. La hora cambia lo que encuentra Link: de día parece una extensión abierta y tranquila, mientras que la noche trae otros peligros. Más adelante, Epona cambia la forma de recorrerla."},
          Classic, false},
    Entry{"kakariko", Kind::Place, {"Kakariko Village", "Aldea Kakariko"},
          {"A village at the foot of Death Mountain with ties to the Sheikah.",
           "Una aldea a los pies de la Montaña de la Muerte vinculada a los Sheikah."},
          {"The village connects Hyrule Field to Death Mountain Trail. Its graveyard hides royal history and music, while its residents provide clues, services and side stories. Returning at different points in the adventure reveals how events have changed everyday life here.",
           "La aldea une la Pradera de Hyrule con la Senda de la Montaña de la Muerte. Su cementerio esconde historia y música de la familia real; sus habitantes ofrecen pistas, servicios e historias propias. Volver en otros momentos de la aventura deja ver cómo cambian sus vidas."},  // codespell:ignore momentos
          Classic, false},
    Entry{"lost_woods", Kind::Place, {"Lost Woods", "Bosque Perdido"},
          {"A maze of forest paths near the Kokiri village.",
           "Un laberinto de senderos próximo a la aldea Kokiri."},
          {"The paths loop deceptively, and following the right melody helps find the way through. The woods connect to the Sacred Forest Meadow and contain hidden meetings and merchants. Stories about children lost among these trees give the area a more unsettling side.",
           "Los senderos engañan y se repiten; seguir la melodía correcta ayuda a encontrar la salida. El bosque conecta con la Pradera Sagrada y esconde encuentros y comerciantes. Los relatos sobre niños perdidos entre los árboles le dan un lado más inquietante."},
          Classic, false},
    Entry{"death_mountain", Kind::Place, {"Death Mountain", "Montaña de la Muerte"},
          {"The volcanic mountain above Kakariko and home to the Gorons.",
           "La montaña volcánica sobre Kakariko, hogar de los Goron."},
          {"The trail climbs from Kakariko toward Goron City and the volcanic crater. Falling rocks, steep paths and heat make the mountain more than a scenic backdrop. The Gorons' home and Dodongo Cavern explain why access to the mountain matters to the kingdom.",
           "La senda sube desde Kakariko hacia la Ciudad Goron y el cráter volcánico. Rocas que caen, pendientes y calor hacen de la montaña algo más que un paisaje. El hogar de los Goron y la Caverna de los Dodongos explican por qué importa el acceso a esta región."},
          Classic, false},
    Entry{"zora_domain", Kind::Place, {"Zora Domain", "Dominio Zora"},
          {"The Zora people's home in Hyrule's waterways.",
           "El hogar del pueblo Zora entre los cursos de agua de Hyrule."},  // codespell:ignore cursos
          {"King Zora presides over this watery domain, and Lord Jabu-Jabu is revered at the nearby fountain. The route from Zora River makes the domain feel secluded despite its place in Hyrule. Its fate in the two periods of Link's adventure gives the region an especially strong sense of change.",  // codespell:ignore revered
           "El rey Zora gobierna este dominio acuático y Lord Jabu-Jabu es venerado en la fuente cercana. La ruta desde el Río Zora hace que parezca aislado a pesar de formar parte de Hyrule. Su destino en los dos periodos de la aventura de Link muestra especialmente bien el paso del tiempo."},  // codespell:ignore hace
          Classic, false},
    Entry{"lon_lon_ranch", Kind::Place, {"Lon Lon Ranch", "Rancho Lon Lon"},
          {"A ranch on Hyrule Field known for its horses and milk.",
           "Un rancho de la Pradera de Hyrule conocido por sus caballos y su leche."},
          {"Malon and Talon help make the ranch a human pause between dangerous regions; Ingo works there too. Epona and her song make this a place worth revisiting, not merely a stop on the road. The ranch also shows how Hyrule's political changes reach ordinary people.",
           "Malon y Talon hacen del rancho una pausa humana entre regiones peligrosas; Ingo también trabaja allí. Epona y su canción convierten el lugar en una visita recurrente, no en una simple parada. El rancho muestra además cómo los cambios políticos de Hyrule afectan a la gente corriente."},
          Classic, false},
    Entry{"link", Kind::Character, {"Link", "Link"},
          {"The young hero whose journey begins in Kokiri Forest.",
           "El joven héroe cuya aventura comienza en el Bosque Kokiri."},
          {"Raised among the Kokiri, Link begins without knowing why he does not quite belong. His quest crosses Hyrule as both a child and an adult, making the passage of time part of the story rather than just a setting. The series later remembers him as the Hero of Time.",
           "Criado entre los Kokiri, Link comienza sin saber por qué no acaba de encajar. Recorre Hyrule como niño y como adulto, de modo que el paso del tiempo forma parte de la historia y no solo del escenario. La saga lo recordará como el Héroe del Tiempo."},
          Classic, false},
    Entry{"navi", Kind::Character, {"Navi", "Navi"},
          {"The fairy who accompanies Link and helps him identify creatures.",
           "El hada que acompaña a Link y le ayuda a identificar criaturas."},
          {"Navi is Link's guide and his link to the Great Deku Tree's wishes. In the Switch 2 remake she describes enemies when asked instead of interrupting so often. The expanded story explains her knowledge: the tree taught her about Hyrule's creatures before she set out with Link.",
           "Navi guía a Link y lo conecta con la voluntad del Gran Árbol Deku. En el remake de Switch 2 describe enemigos cuando se le pregunta en vez de interrumpir tan a menudo. La historia ampliada explica su saber: el árbol le enseñó sobre las criaturas de Hyrule antes de enviarla con Link."},
          Remake, false},
    Entry{"great_deku_tree", Kind::Character, {"Great Deku Tree", "Gran Árbol Deku"},
          {"The guardian of Kokiri Forest.", "El guardián del Bosque Kokiri."},
          {"The tree's illness begins Link's journey and draws him into Ganondorf's plans. He sends Navi to the young hero and explains the danger facing Hyrule. The remake expands his conversations and makes his warning to the Kokiri less mysterious: the world beyond the forest is perilous for them.",
           "La enfermedad del árbol inicia el viaje de Link y lo arrastra a los planes de Ganondorf. Envía a Navi a buscar al joven héroe y le explica el peligro para Hyrule. El remake amplía sus conversaciones y aclara su advertencia a los Kokiri: el mundo fuera del bosque es peligroso para ellos."},
          Remake, false},
    Entry{"saria", Kind::Character, {"Saria", "Saria"},
          {"A Kokiri and one of Link's closest friends.",
           "Una Kokiri y una de las amistades más cercanas de Link."},
          {"Saria treats Link as a friend even when other Kokiri see him as different. Her song becomes a way to keep that bond across distance. Her connection to the Lost Woods and Sacred Forest Meadow makes the forest feel like a place with its own people and history, not only a route to a dungeon.",
           "Saria trata a Link como amigo incluso cuando otros Kokiri lo consideran distinto. Su canción mantiene ese vínculo a distancia. Su relación con el Bosque Perdido y la Pradera Sagrada hace que el bosque tenga personas e historia propias y no sea solo una ruta hacia una mazmorra."},  // codespell:ignore hace
          Classic, false},
    Entry{"zelda", Kind::Character, {"Princess Zelda", "Princesa Zelda"},
          {"The princess of Hyrule, concerned about a threat to the kingdom.",
           "La princesa de Hyrule, preocupada por una amenaza para el reino."},
          {"Zelda senses Ganondorf's threat to the kingdom and seeks help before she can prove every part of his plan. Her meeting with Link sets the search for the Spiritual Stones in motion. The remake gives her more dialogue and explains some choices that the original story left open to interpretation.",
           "Zelda percibe la amenaza de Ganondorf y busca ayuda antes de poder demostrar cada parte de su plan. Su encuentro con Link pone en marcha la búsqueda de las Piedras Espirituales. El remake le da más diálogo y explica decisiones que la historia original dejaba abiertas a interpretación."},
          Remake, false},
    Entry{"impa", Kind::Character, {"Impa", "Impa"},
          {"Zelda's protector and a member of the Sheikah.",
           "Protectora de Zelda y miembro de los Sheikah."},
          {"Impa protects the princess and belongs to the Sheikah, a people with a long connection to Hyrule's royal family. She escorts Link away from the castle after his meeting with Zelda and directs him toward his next task. Her duty to Zelda helps explain why she cannot simply make the whole journey herself.",
           "Impa protege a la princesa y pertenece a los Sheikah, pueblo ligado desde antiguo a la familia real de Hyrule. Acompaña a Link al salir del castillo tras conocer a Zelda y le indica su siguiente cometido. Su deber hacia la princesa ayuda a entender por qué no emprende ella sola todo el viaje."},
          Classic, false},
    Entry{"darunia", Kind::Character, {"Darunia", "Darunia"},
          {"Leader of the Gorons on Death Mountain.",
           "Líder de los Goron en la Montaña de la Muerte."},
          {"Darunia leads a proud mountain people whose survival depends on access to Dodongo Cavern. The crisis there strains his patience and his trust in outsiders. Gaining his respect connects Link to the Gorons and turns a regional problem into part of the kingdom-wide struggle.",
           "Darunia lidera a un pueblo de montaña cuya supervivencia depende del acceso a la Caverna de los Dodongos. La crisis allí pone a prueba su paciencia y su confianza en los forasteros. Ganarse su respeto acerca a Link a los Goron y vincula un problema local con la lucha de todo el reino."},
          Classic, false},
    Entry{"ruto", Kind::Character, {"Princess Ruto", "Princesa Ruto"},
          {"A princess of the Zora people.", "Una princesa del pueblo Zora."},
          {"Ruto's story begins with the illness of Lord Jabu-Jabu and the Spiritual Stone entrusted to her family. Her strong will makes her exchanges with Link memorable. In the remake, a misunderstanding over a letter gives a clearer reason for her initial anger.",
           "La historia de Ruto comienza con la enfermedad de Lord Jabu-Jabu y la Piedra Espiritual confiada a su familia. Su carácter firme hace memorables sus conversaciones con Link. En el remake, un malentendido sobre una carta da una razón más clara a su enfado inicial."},  // codespell:ignore hace,inicial
          Classic, false},
    Entry{"navi_remake", Kind::Remake, {"Navi in the remake", "Navi en el remake"},
          {"Navi offers enemy descriptions on request rather than constant interruptions.",
           "Navi ofrece descripciones de enemigos cuando se le pide, en vez de interrumpir constantemente."},
          {"The expanded dialogue gives an in-world reason for her knowledge: the Great Deku Tree taught her about Hyrule's creatures. Her role remains that of Link's companion, but the player chooses when to ask for many descriptions. This makes her advice feel more like a resource to consult than a constant interruption.",
           "El diálogo ampliado explica su conocimiento dentro del mundo del juego: el Gran Árbol Deku le enseñó sobre las criaturas de Hyrule. Sigue acompañando a Link, pero ahora el jugador decide cuándo pedir muchas descripciones. Así, su consejo se siente como un recurso que consultar y no una interrupción constante."},
          Remake, false},
    Entry{"ganondorf", Kind::Character, {"Ganondorf", "Ganondorf"},
          {"King of the Gerudo and the political threat behind Hyrule's crisis.",
           "Rey de los gerudo y amenaza política detrás de la crisis de Hyrule."},
          {"He seeks the power of the Triforce while presenting himself at the royal court. Several regional disasters bear his influence even when he is not physically present. The remake gives him more formal speech, strengthening the contrast between his public bearing and his ambitions.",
           "Busca el poder de la Trifuerza mientras se presenta en la corte real. Su influencia llega a varias crisis regionales aun cuando no aparece en persona. El remake le da un habla más formal, que acentúa el contraste entre sus modales públicos y sus ambiciones."},
          Remake, false},
    Entry{"malon", Kind::Character, {"Malon", "Malon"},
          {"A young woman of Lon Lon Ranch with a close bond to Epona.",
           "Joven del Rancho Lon Lon muy unida a Epona."},
          {"Malon shares a song that helps Link connect with the horse. Her warmth and work at the ranch provide a gentler story between the kingdom's larger conflicts. Visiting again later shows what became of her family and the ranch.",
           "Malon comparte una canción que ayuda a Link a acercarse a la yegua. Su amabilidad y trabajo en el rancho ofrecen una historia más cercana entre los grandes conflictos del reino. Volver más tarde muestra qué fue de su familia y del lugar."},
          Characters, false},
    Entry{"epona", Kind::Character, {"Epona", "Epona"},
          {"The horse associated with Malon and Link's journeys across Hyrule Field.",
           "La yegua ligada a Malon y a los viajes de Link por la Pradera de Hyrule."},
          {"Epona turns the broad field from a long walk into a ride and makes certain pursuits possible. Her bond with Malon is part of the ranch's identity. Learning Epona's Song helps Link call to her when the time is right.",
           "Epona convierte la amplia pradera en un recorrido a caballo y hace posibles ciertas persecuciones. Su vínculo con Malon forma parte de la identidad del rancho. Aprender la Canción de Epona permite a Link llamarla cuando llegue el momento."},  // codespell:ignore hace,momento
          Characters, false},
    Entry{"talon", Kind::Character, {"Talon", "Talon"},
          {"Malon's father and one of the people running Lon Lon Ranch.",
           "Padre de Malon y uno de los responsables del Rancho Lon Lon."},
          {"His sleepy habits make him memorable, but his family also depends on the ranch. Link's encounters with Talon connect everyday work, Epona and the events affecting the kingdom. He is part of the ordinary life worth protecting in this adventure.",
           "Su afición a dormirse lo hace inolvidable, pero su familia también depende del rancho. Los encuentros de Link con él unen el trabajo diario, Epona y los sucesos que afectan al reino. Representa la vida cotidiana que merece ser protegida."},  // codespell:ignore hace,ser
          Characters, false},
    Entry{"ingo", Kind::Character, {"Ingo", "Ingo"},
          {"A ranch worker whose ambitions complicate life at Lon Lon Ranch.",
           "Trabajador del rancho cuyas ambiciones complican la vida en Lon Lon."},
          {"Ingo feels overshadowed by Talon and wants greater control of the ranch. His role changes as the wider kingdom changes, making the place's politics personal. Epona is central to the conflict that Link eventually finds there.",
           "Ingo se siente eclipsado por Talon y desea controlar el rancho. Su papel cambia al mismo tiempo que cambia el reino, dando una dimensión personal a la política. Epona ocupa un lugar central en el conflicto que Link acaba encontrando allí."},
          Characters, false},
    Entry{"king_zora", Kind::Character, {"King Zora", "Rey Zora"},
          {"Ruler of Zora Domain and Ruto's father.", "Gobernante del Dominio Zora y padre de Ruto."},
          {"His concern for his daughter and for Lord Jabu-Jabu links the Zora crisis to Link's search for a Spiritual Stone. The royal court's worries show that Ganondorf's plans reach beyond the Hylian castle. Talking with him is part of understanding the region's customs.",
           "Su preocupación por la hija y por Lord Jabu-Jabu une la crisis Zora con la búsqueda de una Piedra Espiritual. Los temores de su corte muestran que los planes de Ganondorf van más allá del castillo hyliano. Hablar con él ayuda a comprender las costumbres de la región."},
          Characters, false},
    Entry{"mido", Kind::Character, {"Mido", "Mido"},
          {"A Kokiri who challenges Link's place in the forest community.",
           "Kokiri que cuestiona el lugar de Link dentro de la comunidad del bosque."},
          {"He guards access to the Great Deku Tree and takes his own role very seriously. His friction with Link hints that the young hero is different from his neighbors. Returning to the forest later gives their relationship more context.",
           "Guarda el acceso al Gran Árbol Deku y se toma muy en serio su cometido. Su roce con Link sugiere que el joven héroe es distinto a sus vecinos. Volver al bosque más adelante da otro contexto a su relación."},
          Characters, false},
    Entry{"kaepora", Kind::Character, {"Kaepora Gaebora", "Kaepora Gaebora"},
          {"A wise owl who offers guidance across Hyrule.",
           "Búho sabio que ofrece orientación por Hyrule."},
          {"His appearances punctuate journeys between important places. In the Switch 2 remake he also guides the Hall of Fallen Foes, where defeated creatures can be studied and bosses challenged again. His advice can be long, but often points toward the next goal.",
           "Sus apariciones acompañan los viajes entre lugares importantes. En el remake de Switch 2 también guía el Salón de los Enemigos Caídos, donde estudiar criaturas derrotadas y repetir jefes. Sus consejos pueden ser largos, pero suelen señalar el siguiente objetivo."},  // codespell:ignore ser
          Characters, false},
    Entry{"dampe", Kind::Character, {"Dampé", "Dampé"},
          {"The gravedigger of Kakariko's cemetery.", "El sepulturero del Cementerio de Kakariko."},
          {"Dampé is tied to the cemetery's graves, hidden routes and a memorable challenge. His work gives the place a human presence among the ghosts. Revisiting the cemetery at another time reveals more of his story.",
           "Dampé está ligado a las tumbas, rutas ocultas y un reto memorable del cementerio. Su oficio aporta una presencia humana entre los fantasmas. Volver al cementerio en otra época descubre más de su historia."},
          Characters, false},
    Entry{"biggoron", Kind::Character, {"Biggoron", "Biggoron"},
          {"An enormous Goron craftsman high on Death Mountain.",
           "Artesano Goron gigantesco en las alturas de la Montaña de la Muerte."},
          {"His skill is linked to a long sequence of trades and the Biggoron Sword. The quest encourages revisiting people and regions that might otherwise be forgotten after the main story moves on. He gives the mountain another identity beyond Darunia's city.",
           "Su oficio está unido a una larga cadena de intercambios y a la Espada de Biggoron. La misión invita a volver a personas y regiones que podrían olvidarse al avanzar la historia principal. Da a la montaña otra identidad además de la ciudad de Darunia."},
          Characters, false},
    Entry{"jabu_jabu", Kind::Character, {"Lord Jabu-Jabu", "Lord Jabu-Jabu"},
          {"A great creature revered by the Zora people.",  // codespell:ignore revered
           "Gran criatura venerada por el pueblo Zora."},
          {"Jabu-Jabu lives at Zora Fountain and his illness alarms the domain. Link's journey inside him turns a revered figure into a living dungeon. Ruto's connection to him makes the rescue personal as well as important to the wider quest.",  // codespell:ignore revered
           "Jabu-Jabu vive en la Fuente Zora y su enfermedad alarma al dominio. El viaje de Link por su interior convierte a una figura venerada en una mazmorra viva. La relación de Ruto con él hace del rescate algo personal además de importante para la misión."},  // codespell:ignore hace
          Characters, false},
    Entry{"sheik", Kind::Character, {"Sheik", "Sheik"},
          {"A mysterious guide encountered in the adult era.",
           "Guía misterioso que aparece en la era adulta."},
          {"Sheik teaches melodies that open routes and connect the temples to the larger struggle. The character's identity is a central story reveal, so this card describes the role without naming who is behind it. Listen to the advice at each meeting: it often points to a region's next step.",
           "Sheik enseña melodías que abren rutas y relacionan los templos con la lucha general. Su identidad es una revelación importante, por lo que esta ficha describe su papel sin decir quién se oculta detrás. Escucha sus consejos: suelen indicar el siguiente paso en una región."},
          Characters, false},
    Entry{"rauru", Kind::Character, {"Rauru", "Rauru"},
          {"A Sage associated with the Temple of Light and the Sacred Realm.",
           "Sabio vinculado al Templo de la Luz y al Reino Sagrado."},
          {"Rauru explains the gap between Link's childhood and adulthood and the consequences of opening the way to the Master Sword. His account connects the personal quest to the Sages' shared task. The remake also clarifies his role in protecting other Sages.",
           "Rauru explica el salto entre la infancia y la edad adulta de Link y las consecuencias de abrir el camino a la Espada Maestra. Su relato une la misión personal con la tarea de los Sabios. El remake también aclara su papel al proteger a otros Sabios."},
          Characters, true},
    Entry{"nabooru", Kind::Character, {"Nabooru", "Nabooru"},
          {"A Gerudo who openly resists Ganondorf's ambitions.",
           "Gerudo que se opone abiertamente a las ambiciones de Ganondorf."},
          {"Her defiance shows that the Gerudo people are not united behind their king's actions. The Spirit Temple ties her story to Link's journey across two ages. The remake expands why she takes such a dangerous stand largely on her own.",
           "Su resistencia muestra que el pueblo gerudo no apoya unánimemente las acciones de su rey. El Templo del Espíritu une su historia al viaje de Link por dos edades. El remake amplía por qué asume una postura tan peligrosa casi en solitario."},  // codespell:ignore asume
          Characters, true},
    Entry{"goron_speech", Kind::Remake, {"Goron speech", "El habla de los Goron"},
          {"Gorons end some sentences with '-Goro' in this remake.",
           "Los Goron terminan algunas frases con «-Goro» en este remake."},
          {"This verbal tic was absent from the Nintendo 64 and Nintendo 3DS versions. It gives Goron dialogue a recognizable voice even when different individuals speak. It is a small writing change, but one that makes the mountain community feel more distinct.",
           "Esta muletilla no aparecía en las versiones de Nintendo 64 y Nintendo 3DS. Da al habla Goron una voz reconocible incluso cuando conversan individuos distintos. Es un cambio pequeño en la escritura, pero hace más singular a la comunidad de la montaña."},  // codespell:ignore hace
          Remake, false},
    Entry{"voiced_scenes", Kind::Remake, {"Expanded cutscenes", "Escenas ampliadas"},
          {"The Switch 2 remake expands dialogue and gives important scenes full voice acting.",
           "El remake de Switch 2 amplía los diálogos y da voz a escenas importantes."},
          {"Nintendo also describes reimagined visuals, symphonic music and reworked camera and movement controls. Added dialogue gives familiar scenes more room to explain character motives. These presentation changes do not move Ocarina of Time to a new point in Zelda's story chronology.",
           "Nintendo también destaca gráficos renovados, música sinfónica y cambios en la cámara y el movimiento. Los diálogos añadidos permiten a escenas conocidas explicar mejor las motivaciones. Estos cambios de presentación no trasladan Ocarina of Time a otro punto de la cronología argumental."},
          Nintendo, false},
    Entry{"deku_tree_dungeon", Kind::Dungeon, {"Great Deku Tree", "Gran Árbol Deku: mazmorra"},
          {"Link's first dungeon lies within the Great Deku Tree.",
           "La primera mazmorra de Link está dentro del Gran Árbol Deku."},
          {"Webs, climbable walls and Skulltulas introduce the pattern of observing a room before crossing it. The Fairy Slingshot lets Link reach targets beyond his sword. The danger at the end connects the dungeon to the illness of the tree that shelters the Kokiri.",
           "Telarañas, paredes escalables y Skulltulas enseñan a observar una sala antes de cruzarla. El tirachinas permite alcanzar objetivos lejanos. El peligro del final une la mazmorra con la enfermedad del árbol que protege a los Kokiri."},
          Classic, false},
    Entry{"forest_temple", Kind::Dungeon, {"Forest Temple", "Templo del Bosque"},
          {"A later dungeon connected to the forest.",
           "Una mazmorra posterior vinculada al bosque."},
          {"The temple belongs to the adult part of Link's journey and lies beyond the Sacred Forest Meadow. Its four Poe sisters turn missing light into a puzzle through much of the building. Stalfos battles and the bow prepare Link for the fight waiting in the deepest room.",
           "El templo pertenece a la etapa adulta de Link y se encuentra más allá de la Pradera Sagrada. Sus cuatro hermanas Poe convierten la luz desaparecida en un puzle que recorre el edificio. Los combates con Stalfos y el arco preparan a Link para el enemigo de la sala más profunda."},
          Classic, true},
    Entry{"timeline_position", Kind::Timeline, {"Ocarina in the timeline", "Ocarina en la cronología"},
          {"Ocarina of Time stands at the end of the shared Hero of Time era, before the series branches into different histories.",
           "Ocarina of Time culmina la era común del Héroe del Tiempo, antes de que la saga se divida en distintas historias."},
          {"Shared history: Skyward Sword → The Minish Cap → Four Swords → Ocarina of Time.\n\nThis is story order, not release order. The Switch 2 remake retells Ocarina's chapter; its new release date does not move its events after newer games. Nintendo calls this point the Era of the Hero of Time.",
           "Historia común: Skyward Sword → The Minish Cap → Four Swords → Ocarina of Time.\n\nEs el orden de la historia, no el de publicación. El remake de Switch 2 vuelve a contar este capítulo: su nueva fecha de estreno no sitúa sus acontecimientos después de juegos más recientes. Nintendo llama a este momento la Era del Héroe del Tiempo."},  // codespell:ignore momento
          Timeline, false},
    Entry{"timeline_split", Kind::Timeline, {"The three branches", "Las tres ramas temporales"},
          {"Nintendo's official chronology branches at the outcome of Ocarina of Time.",
           "La cronología oficial de Nintendo se ramifica según el desenlace de Ocarina of Time."},  // codespell:ignore oficial
          {"If the hero is defeated → A Link to the Past → Link's Awakening and later adventures.\n\nIf he wins, the child-era branch → Majora's Mask → Twilight Princess → Four Swords Adventures.\n\nThe adult-era branch → The Wind Waker → Phantom Hourglass → Spirit Tracks.\n\nThese are alternate histories after Ocarina of Time, not consecutive chapters of a single line. Nintendo's official interactive chronology shows the full sequence.",
           "Si el héroe cae derrotado → A Link to the Past → Link's Awakening y otras aventuras.\n\nSi vence, la rama de Link niño → Majora's Mask → Twilight Princess → Four Swords Adventures.\n\nLa rama de Link adulto → The Wind Waker → Phantom Hourglass → Spirit Tracks.\n\nSon historias alternativas después de Ocarina of Time, no capítulos consecutivos de una sola línea. La cronología interactiva oficial de Nintendo muestra la secuencia completa."},  // codespell:ignore oficial
          Timeline, true},
    Entry{"impa_remake", Kind::Remake, {"Impa's decision", "La decisión de Impa"},
          {"The remake explains why Impa asks Link to seek the remaining Spiritual Stones.",
           "El remake explica por qué Impa pide a Link que busque las demás Piedras Espirituales."},
          {"Impa cannot leave Zelda unprotected for long, and escorts Link out in case Ganondorf has allies among the castle staff. This answers an old question about why she sends a child to seek the Stones instead of leaving the princess to do it herself. Her choice reflects a duty to guard Zelda while danger is already inside the court.",
           "Impa no puede dejar a Zelda desprotegida mucho tiempo y acompaña a Link fuera por si Ganondorf tiene aliados entre el personal del castillo. Esto responde a la vieja pregunta de por qué envía a un niño a buscar las Piedras en vez de ir ella. Su decisión refleja el deber de cuidar a Zelda mientras el peligro ya está en la corte."},  // codespell:ignore responde
          Remake, true},
    Entry{"ruto_remake", Kind::Remake, {"Ruto's first reaction", "La reacción inicial de Ruto"},  // codespell:ignore inicial
          {"The remake gives a reason for Ruto's anger when she first meets Link.",
           "El remake da una razón para el enfado de Ruto cuando conoce a Link."},
          {"Navi reveals within Ruto's hearing that Link showed her letter to her father, despite Ruto asking him not to. Ruto's sharp reaction is therefore tied to a specific breach of trust rather than unexplained hostility. The extra exchange changes how their first meeting reads without changing who Ruto is.",
           "Navi revela delante de Ruto que Link mostró su carta al padre de ella, pese a que Ruto le había pedido que no lo hiciera. Su reacción brusca responde así a una falta de confianza concreta y no a una hostilidad sin explicación. La conversación cambia cómo se entiende el primer encuentro sin cambiar quién es Ruto."},  // codespell:ignore responde
          Remake, true},
    Entry{"sages_remake", Kind::Remake, {"The Six Sages", "Los Seis Sabios"},
          {"The remake clarifies what happened to the Sages during their ordeals.",
           "El remake aclara qué les ocurrió a los Sabios durante sus pruebas."},
          {"It says that Rauru took the Sages away before they could be killed, resolving an ambiguity in the original story. Older versions left enough room for players to wonder whether becoming a Sage meant dying. The remake makes their survival an intentional act of protection within the plot.",
           "Explica que Rauru apartó a los Sabios antes de que pudieran morir y resuelve una ambigüedad de la historia original. Las versiones anteriores permitían preguntarse si convertirse en Sabio implicaba morir. El remake presenta su supervivencia como un acto deliberado de protección dentro de la trama."},  // codespell:ignore trama
          Remake, true},
};

QString Fold(QString text) {
    text = text.normalized(QString::NormalizationForm_D).toCaseFolded();
    QString result;
    result.reserve(text.size());
    for (const QChar character : text) {
        if (character.category() != QChar::Mark_NonSpacing &&
            character.category() != QChar::Mark_SpacingCombining &&
            character.category() != QChar::Mark_Enclosing) {
            result.append(character);
        }
    }
    return result;
}

} // namespace

std::span<const Entry> OcarinaEntries() {
    static const std::vector<Entry> all = [] {
        const auto enemies = BestiaryEntries();
        const auto world = WorldEntries();
        const auto items = ItemEntries();
        std::vector<Entry> result;
        result.reserve(Entries.size() + enemies.size() + world.size() + items.size());
        result.insert(result.end(), Entries.begin(), Entries.end());
        result.insert(result.end(), enemies.begin(), enemies.end());
        result.insert(result.end(), world.begin(), world.end());
        result.insert(result.end(), items.begin(), items.end());
        return result;
    }();
    return all;
}

const Game& Ocarina() {
    static constexpr std::array kinds{Kind::All,  Kind::Place,   Kind::Character,
                                      Kind::Enemy, Kind::Boss,   Kind::Dungeon,
                                      Kind::Item, Kind::Remake, Kind::Timeline};
    static const Game game{OcarinaSwitch2ProgramId,
                           {"Ocarina of Time", "Ocarina of Time"},
                           {"Explore Hyrule by topic. Loaded scenes may suggest a card.",
                            "Explora Hyrule por temas. Los escenarios cargados pueden sugerir una ficha."},
                           {"Items and songs", "Objetos y canciones"},
                           OcarinaEntries,
                           kinds,
                           OcarinaEntryForScene};
    return game;
}

const Game* FindGame(u64 program_id) {
    for (const Game* game : {&Ocarina(), &BreathOfTheWild()}) {
        if (game->program_id == program_id) {
            return game;
        }
    }
    return nullptr;
}

const Entry* FindEntry(const Game& game, std::string_view id) {
    for (const Entry& entry : game.entries()) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

const Entry* EntryForScene(const Game& game, std::string_view scene) {
    return scene.empty() ? nullptr : game.entry_for_scene(scene);
}

const Entry* OcarinaEntryForScene(std::string_view scene) {
    for (const std::string_view suffix : {"_Adult_Alternative", "_Adult", "_Child"}) {
        if (scene.ends_with(suffix)) {
            scene.remove_suffix(suffix.size());
            break;
        }
    }
    struct SceneEntry {
        std::string_view scene;
        std::string_view entry;
    };
    static constexpr SceneEntry known[] = {
        {"BarinadeStage", "barinade"},
        {"BigOctoStage", "big_octo"},
        {"BongoBongoStage", "bongo_bongo"},
        {"BottomOfTheWell", "bottom_of_well"},
        {"CrumblingGanonCastle", "inside_ganondorf_castle"},
        {"DarkLinkStage", "dark_link"},
        {"DeadHandStage", "dead_hand"},
        {"DeathMountainCrater", "death_mountain_crater"},
        {"DeathMountainTrail", "death_mountain_trail"},
        {"DesertColossus", "desert_colossus"},
        {"DodongoCavern", "dodongo_cavern"},
        {"DodongoStage", "king_dodongo"},
        {"EnemyMuseum", "hall_of_foes"},
        {"FireTemple", "fire_temple"},
        {"FlareDancerStage", "flare_dancer"},
        {"ForestTemple", "forest_temple"},
        {"GanonCastle", "ganondorf_castle"},
        {"GanonStage", "ganon_boss"},
        {"GanondorfStage", "ganondorf_boss"},
        {"GerudoFortress", "gerudo_fortress"},
        {"GerudoTrainingGround", "gerudo_proving_grounds"},
        {"GerudoValley", "gerudo_valley"},
        {"GohmaStage", "gohma"},
        {"GoronCity", "goron_city"},
        {"Graveyard", "graveyard"},
        {"HyruleCastle", "hyrule_castle"},
        {"HyruleField", "hyrule_field"},
        {"IceCavern", "ice_cavern"},
        {"InsideGanonCastle", "inside_ganondorf_castle"},
        {"InsideJabuJabuBelly", "jabu_jabu_belly"},
        {"InsideTheDekuTree", "deku_tree_dungeon"},
        {"IronKnuckleBlackStage", "iron_knuckle"},
        {"IronKnuckleWhiteStage", "iron_knuckle"},
        {"KakarikoVillage", "kakariko"},
        {"KokiriForest", "kokiri_forest"},
        {"LostWood", "lost_woods"},
        {"Market", "castle_town"},
        {"MorphaStage", "morpha"},
        {"PhantomGanonStage", "phantom_ganon"},
        {"SacredForestMeadow", "sacred_grove"},
        {"SageRoom", "chamber_of_sages"},
        {"ShadowTemple", "shadow_temple"},
        {"SpiritTemple", "spirit_temple"},
        {"TwinrovaStage", "twinrova"},
        {"VolvagiaStage", "volvagia"},
        {"WaterTemple", "water_temple"},
        {"ZoraDomain", "zora_domain"},
        {"ZoraFountain", "zora_fountain"},
        {"ZoraRiver", "zora_river"},
    };
    for (const auto& mapping : known) {
        if (scene == mapping.scene) {
            return FindEntry(Ocarina(), mapping.entry);
        }
    }
    return nullptr;
}

QString Localize(Text text, bool spanish) {
    const auto value = spanish ? text.es : text.en;
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

std::vector<const Entry*> Search(const Game& game, QString query, Kind kind, bool spoilers) {
    const QStringList terms = Fold(query.simplified()).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    std::vector<const Entry*> matches;
    for (const Entry& entry : game.entries()) {
        if ((!spoilers && entry.spoiler) || (kind != Kind::All && entry.kind != kind)) {
            continue;
        }
        const QString haystack = Fold(QString::fromUtf8(entry.id.data(), entry.id.size()) +
                                      Localize(entry.title, false) + Localize(entry.title, true) +
                                      Localize(entry.summary, false) + Localize(entry.summary, true) +
                                      Localize(entry.detail, false) + Localize(entry.detail, true));
        bool found = true;
        for (const QString& term : terms) {
            if (!haystack.contains(term)) {
                found = false;
                break;
            }
        }
        if (found) {
            matches.push_back(&entry);
        }
    }
    return matches;
}

} // namespace GameCompanion
