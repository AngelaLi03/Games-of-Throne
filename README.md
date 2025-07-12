# 🏰 Games of Throne – A 2D RPG Adventure (C++ & OpenGL)

**Games of Throne** is a custom-built 2D RPG powered by C++ and OpenGL, featuring real-time combat and AI-driven enemy behaviors. The game offers a fully interactive experience with unique bosses, ability systems, and visually rich levels.

## 🎮 Game Story: The Rise of the Usurper
 
You are a rogue assassin born in the shadows of the kingdom. With a corrupt king on the throne and a city suffering under tyranny, your mission is clear: eliminate every royal guard, confront the prince, and assassinate the king to reclaim justice. But the deeper you go, the more you uncover—secrets, betrayals, and a past even you didn't know.

## ✨ Key Features

### Interactions

Dialogues and popups are navigated with Enter. These occur at moments: when the game starts, when the first minion is killed, when the Chef is first attacked, and when the Chef is defeated.
Treasure boxes and fountains interacted with E (open boxes, heal fully, collect items).
Treasure boxes spawn random weapons.

### Minion AI:
- A\* pathfinding to follow the player, visible in debug mode.
- Minions attack when within range.

### 🔥 Combat and Abilities
- Light attacks with Left Mouse Click (energy required, weapon damage applied).
- Flow charge with Right Mouse Click (Hold) and heavy attack on release (high damage, weapon-specific animations).
- Sprinting, dodging (with invincibility frames), and a perfect dodge mechanic with flow rewards
- S mouse gesture enlarges the player and player hitbox for 3 seconds and then returns to normal

### 💥 Enemies and Bosses
- A* pathfinding minions with attack mechanics
- New ranged minion type
- Chef Boss with tomato/pan throws, dashes, and unique dialogues
- Knight Boss with 3 attack patterns, shield counter, and skinned motion animations
- Prince and King Bosses with distinct mechanics and custom animations

### 🌍 Levels and Interactions
- 4 playable levels with unique layouts and pop-up UIs
- New treasure chests, fountains, and random weapon drops
- Chest unlocks after defeating all nearby enemies
- Health/energy persistence across levels and game sessions

### 🗺️ Visual and Audio
- Enhanced level backgrounds and UI overlays
- New font and styled dialogue popups
- 4 original level-specific soundtracks
- Render order and view culling optimizations

### 🧪 Debug and Developer Tools
- Toggle debug mode with bounding boxes, AI paths, hitboxes (I)
- Show FPS (P), help text (O), and pause (0)
- Debug-only hotkeys: B (switch weapons), R (reset game), H (auto-kill boss)


## 📁 Project Structure

- `src/`: Source code for gameplay, rendering, physics, and AI
- `assets/`: Sprites, sounds, fonts, and animation data
- `doc/`: Test plans and design documents


## 📺 Demo & Screenshots

![Game Screenshot](path-to-screenshot.png)  
_Watch the [demo video](https://youtu.be/4JgAWVdJBeU?si=wZQf9B-B436YYSB3) for full gameplay highlights._


## 🤝 Contributions

Created by Angela Li and collaborators.
Game mechanics, assets, AI systems, and animations are all handcrafted for a cohesive experience.
