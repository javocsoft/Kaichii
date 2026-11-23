// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/game_companion_catalog.h"

#include <array>

namespace GameCompanion {
namespace {

constexpr std::string_view Items = "https://zelda.mtwildwood.net/en-gb/items/";
constexpr std::string_view Nintendo =
    "https://www.nintendo.com/us/whatsnew/its-time-experience-the-legend-of-zelda-ocarina-of-time-reborn-exclusively-for-nintendo-switch-2/";

constexpr std::array Entries{
    Entry{"master_sword", Kind::Item, {"Master Sword", "Espada Maestra"},
          {"The legendary blade kept within the Temple of Time.",
           "La espada legendaria custodiada en el Templo del Tiempo."},
          {"Drawing it is one of the story's turning points and binds Link's quest to the Sacred Realm. It is more than an upgrade in damage: the blade marks the transition between the hero's two eras. Its role in the final struggle gives the title Hero of Time its weight.",
           "Desenvainarla marca un giro de la historia y vincula la aventura de Link con el Reino Sagrado. Es más que una mejora de daño: señala el paso entre las dos épocas del héroe. Su papel en la lucha final da sentido al título de Héroe del Tiempo."},
          Items, true},
    Entry{"ocarina_of_time", Kind::Item, {"Ocarina of Time", "Ocarina del Tiempo"},
          {"A sacred instrument used to play melodies that change the world.",
           "Instrumento sagrado con el que se tocan melodías que cambian el mundo."},
          {"The ocarina's songs can call Epona, change the hour, reveal routes and connect distant places. The instrument also has a direct role at the Temple of Time. In the Switch 2 remake, Nintendo says players can use the system microphone to perform notes by humming or playing an instrument.",
           "Sus canciones pueden llamar a Epona, cambiar la hora, revelar rutas y conectar lugares lejanos. También cumple una función directa en el Templo del Tiempo. Nintendo indica que en el remake de Switch 2 se pueden tocar notas tarareando o usando un instrumento ante el micrófono de la consola."},
          Nintendo, false},
    Entry{"fairy_slingshot", Kind::Item, {"Fairy Slingshot", "Tirachinas de las Hadas"},
          {"Child Link's early ranged weapon, firing Deku Seeds.",
           "Primera arma a distancia de Link niño, que dispara semillas Deku."},
          {"It reaches switches and airborne foes that a short sword cannot. Skullwalltulas, Keese and Queen Gohma are good reasons to keep seeds ready. The slingshot is a simple example of how an item can solve both puzzles and fights.",
           "Alcanza interruptores y enemigos voladores fuera del alcance de la espada corta. Skullwalltulas, Keese y la Reina Gohma son buenas razones para llevar semillas. Muestra cómo un mismo objeto resuelve puzles y combates."},
          Items, false},
    Entry{"boomerang", Kind::Item, {"Boomerang", "Bumerán"},
          {"A returning weapon useful against the creatures inside Jabu-Jabu.",
           "Arma que regresa a la mano, útil contra las criaturas dentro de Jabu-Jabu."},
          {"Its reach and stunning effect make electric Biri and Bari safer to handle. It can also retrieve or activate things beyond Link's immediate reach. Against Barinade, controlling distance matters as much as dealing damage.",
           "Su alcance y capacidad de aturdir ayudan a combatir sin tocar a los eléctricos Biri y Bari. También puede recuperar o activar cosas alejadas. Frente a Barinade, controlar la distancia importa tanto como hacer daño."},
          Items, false},
    Entry{"bomb", Kind::Item, {"Bomb", "Bomba"},
          {"An explosive tool for cracked walls, obstacles and armored foes.",
           "Explosivo para muros agrietados, obstáculos y enemigos acorazados."},
          {"Dodongo Cavern teaches many of its uses, including a boss opening. A bomb can destroy a Beamos at its base or reveal a concealed path. Place it with enough time to step clear; the blast affects Link too.",
           "La Caverna de los Dodongos enseña muchos de sus usos, incluso contra un jefe. Puede destruir un Beamos en la base o abrir un camino oculto. Colócala con tiempo para apartarte: la explosión también afecta a Link."},
          Items, false},
    Entry{"hookshot", Kind::Item, {"Hookshot", "Gancho"},
          {"A chain tool that pulls Link to targets or draws objects toward him.",
           "Herramienta con cadena que lleva a Link hasta un objetivo o atrae objetos."},
          {"It changes how Link crosses gaps and approaches otherwise unreachable places. Some enemies can be stunned or exposed with it, including the Flare Dancer and Shell Blade. Look for appropriate surfaces before assuming a gap requires another route.",
           "Cambia la forma de cruzar huecos y llegar a lugares inaccesibles. Puede aturdir o exponer a ciertos enemigos, como el Bailarín ígneo y el Shell Blade. Busca superficies compatibles antes de dar por hecho que un abismo obliga a tomar otro camino."},
          Items, false},
    Entry{"longshot", Kind::Item, {"Longshot", "Gancho largo"},
          {"An extended-range version of the Hookshot.",
           "Versión de mayor alcance del gancho."},
          {"The extra distance opens routes that previously looked possible but remained out of reach. The Water Temple makes its range particularly relevant. It is worth reconsidering earlier rooms after acquiring it, as their geometry has not changed but Link's reach has.",
           "La mayor distancia abre rutas que antes parecían posibles pero quedaban fuera de alcance. Su recorrido importa especialmente en el Templo del Agua. Conviene revisar salas anteriores: su geometría no cambia, pero sí el alcance de Link."},
          Items, true},
    Entry{"fairy_bow", Kind::Item, {"Fairy Bow", "Arco de las Hadas"},
          {"Adult Link's versatile ranged weapon.",
           "Versátil arma a distancia de Link adulto."},
          {"It strikes enemies, switches and distant targets with greater precision than a close attack. The Poe sisters and Phantom Ganon make the bow central to the Forest Temple. Elemental arrows later add new uses to the same weapon.",
           "Alcanza enemigos, interruptores y objetivos lejanos con más precisión que un ataque cercano. Las hermanas Poe y Ganon Fantasma hacen del arco una pieza central del Templo del Bosque. Las flechas elementales añaden después nuevos usos."},
          Items, false},
    Entry{"megaton_hammer", Kind::Item, {"Megaton Hammer", "Martillo Megatón"},
          {"A heavy hammer used to break obstacles and stun hard targets.",
           "Martillo pesado que rompe obstáculos y aturde objetivos duros."},
          {"The Fire Temple makes its force essential. It can open blocked passages, affect stone or fiery enemies and expose Volvagia. Its slower rhythm rewards a deliberate strike rather than a flurry of attacks.",
           "Su fuerza resulta esencial en el Templo del Fuego. Abre pasos bloqueados, afecta a enemigos de piedra o fuego y expone a Volvagia. Su ritmo más lento premia un golpe medido en vez de una ráfaga de ataques."},
          Items, true},
    Entry{"lens_of_truth", Kind::Item, {"Lens of Truth", "Lente de la Verdad"},
          {"A magical lens that reveals hidden paths and concealed things.",
           "Lente mágica que revela caminos y cosas ocultas."},
          {"It is closely associated with the Bottom of the Well and Shadow Temple. Use it when the geometry seems impossible or an enemy disappears without an obvious escape. Its use consumes magic, so look carefully and switch it off when you understand the route.",
           "Está muy ligada al Fondo del Pozo y al Templo de las Sombras. Úsala cuando un camino parezca imposible o un enemigo desaparezca sin salida visible. Consume magia: observa con atención y apágala cuando ya entiendas la ruta."},  // codespell:ignore imposible
          Items, true},
    Entry{"mirror_shield", Kind::Item, {"Mirror Shield", "Escudo Espejo"},
          {"A shield that reflects light and certain magical attacks.",
           "Escudo que refleja la luz y ciertos ataques mágicos."},
          {"The Spirit Temple turns reflected beams into puzzle solutions. Its role becomes even more dramatic against Twinrova's fire and ice. Consider the angle and the element arriving at the shield rather than treating it as a stronger ordinary guard.",
           "El Templo del Espíritu convierte los rayos reflejados en soluciones de puzles. Contra Twinrova y sus hechizos de fuego y hielo cobra aún más importancia. Piensa en el ángulo y el elemento que recibe, no lo uses solo como defensa ordinaria más fuerte."},
          Items, true},
    Entry{"iron_boots", Kind::Item, {"Iron Boots", "Botas de Hierro"},
          {"Heavy boots that let Link sink and walk underwater.",
           "Botas pesadas que permiten a Link hundirse y caminar bajo el agua."},
          {"They change the Water Temple from a place to swim through into one with routes along its floor. Their weight also alters movement elsewhere. Pair them with the Zora Tunic when an underwater journey will take time.",
           "Transforman el Templo del Agua: ya no solo se nada, también hay rutas por el fondo. Su peso cambia el movimiento en otros lugares. Combínalas con la Túnica Zora si la estancia bajo el agua va a ser larga."},  // codespell:ignore ser
          Items, true},
    Entry{"hover_boots", Kind::Item, {"Hover Boots", "Botas Flotantes"},
          {"Boots that briefly carry Link over gaps before he falls.",
           "Botas que sostienen brevemente a Link sobre un vacío antes de caer."},
          {"They help cross short missing sections of floor in the Shadow Temple. Their sliding feel also reduces precise footing, so equip them when their special movement is needed. Read the distance before stepping out: they are no substitute for a long bridge.",
           "Ayudan a cruzar huecos breves del suelo en el Templo de las Sombras. Su deslizamiento reduce la precisión al pisar; conviene usarlas cuando haga falta su efecto. Calcula la distancia antes de avanzar: no sustituyen a un puente largo."},
          Items, true},
    Entry{"light_arrows", Kind::Item, {"Light Arrows", "Flechas de Luz"},
          {"Sacred arrows important to the final confrontation.",
           "Flechas sagradas importantes en el enfrentamiento final."},
          {"They can stun powerful enemies and create openings against Ganondorf and Ganon. Their role is tied to Zelda and the Sages' shared effort. Keep enough magic available to use them when an opportunity appears.",
           "Pueden aturdir a enemigos poderosos y abrir huecos contra Ganondorf y Ganon. Su papel está unido a Zelda y al esfuerzo conjunto de los Sabios. Conserva magia suficiente para aprovechar una oportunidad cuando aparezca."},
          Items, true},
    Entry{"zelda_lullaby", Kind::Item, {"Zelda's Lullaby", "Nana de Zelda"},
          {"A royal melody that proves Link's connection to Zelda.",
           "Melodía real que acredita la relación de Link con Zelda."},
          {"The song can activate places associated with the royal family. It is one of the earliest examples of music acting like a key in Hyrule. When a symbol of the crown appears, remember the melody before searching for a mechanical switch.",
           "La canción puede activar lugares vinculados a la familia real. Es uno de los primeros ejemplos de música que funciona como llave en Hyrule. Si ves un emblema de la corona, recuerda la melodía antes de buscar un interruptor mecánico."},
          Items, false},
    Entry{"saria_song", Kind::Item, {"Saria's Song", "Canción de Saria"},
          {"A melody representing Link's friendship with Saria.",
           "Melodía que representa la amistad de Link con Saria."},
          {"Its sound helps guide the way through the Lost Woods and can connect Link with Saria from afar. The song turns a personal relationship into a practical tool. Remember it when the forest's branching paths begin to feel identical.",
           "Su sonido ayuda a orientarse en el Bosque Perdido y permite conectar con Saria a distancia. La canción convierte una relación personal en herramienta útil. Recuérdala cuando los cruces del bosque empiecen a parecer iguales."},
          Items, false},
    Entry{"epona_song", Kind::Item, {"Epona's Song", "Canción de Epona"},
          {"The ranch melody that calls to Epona.",
           "Melodía del rancho que llama a Epona."},
          {"Malon teaches it, giving Link a lasting connection to Lon Lon Ranch. Its value becomes clear when horseback travel and pursuit matter. The tune also carries the calmer atmosphere of the ranch into journeys across Hyrule.",
           "Malon la enseña y da a Link un vínculo duradero con el Rancho Lon Lon. Su utilidad aparece cuando importan los viajes y persecuciones a caballo. La melodía lleva algo de la calma del rancho a los caminos de Hyrule."},  // codespell:ignore importan
          Items, false},
    Entry{"suns_song", Kind::Item, {"Sun's Song", "Canción del Sol"},
          {"A melody associated with the royal tombs and the passage of day and night.",
           "Melodía ligada a las tumbas reales y al paso del día a la noche."},
          {"It can change the time of day, making certain meetings or routes easier to arrange. Against ReDeads and Gibdos it has another use: holding them still briefly. The composer brothers' story gives this practical song a place in Hyrule's past.",
           "Puede cambiar la hora y facilitar ciertos encuentros o rutas. Contra ReDeads y Gibdos tiene otro uso: paralizarlos un momento. La historia de los hermanos compositores da a esta canción práctica un lugar en el pasado de Hyrule."},  // codespell:ignore momento
          Items, false},
    Entry{"song_of_time", Kind::Item, {"Song of Time", "Canción del Tiempo"},
          {"A sacred melody linked to the Temple of Time.",
           "Melodía sagrada vinculada al Templo del Tiempo."},
          {"The song, ocarina and Spiritual Stones together matter to the entrance to the Master Sword. Its significance is narrative as well as mechanical: opening that way changes Link's journey. The name also captures the story's central idea of two ages of Hyrule.",
           "La canción, la ocarina y las Piedras Espirituales intervienen en el acceso a la Espada Maestra. Su importancia es narrativa además de mecánica: abrir ese camino cambia el viaje de Link. Su nombre resume la idea central de las dos edades de Hyrule."},
          Items, true},
    Entry{"song_of_storms", Kind::Item, {"Song of Storms", "Canción de las Tormentas"},
          {"A rain-bringing melody tied to Kakariko's windmill.",
           "Melodía que atrae la lluvia y está ligada al molino de Kakariko."},
          {"The musician at the windmill connects the song to an odd time-loop story. Playing it changes the environment and opens the way to a hidden part of Kakariko. It is a good example of Ocarina's playful treatment of cause and effect across ages.",
           "El músico del molino la une a una curiosa historia de bucle temporal. Tocarla cambia el entorno y abre el paso a una parte oculta de Kakariko. Es un buen ejemplo de cómo Ocarina juega con causa y efecto entre épocas."},
          Items, true},
};

} // namespace

std::span<const Entry> ItemEntries() {
    return Entries;
}

} // namespace GameCompanion
