Game Definition and Rules
Overview
In this 4x4 board game, players move meeples across a cyclic board filled with different types of cells. Each cell impacts the players differently, offering points, penalties, mini-games, or other unique events. 
The first player to reach 100 points wins the game.
Game Board Setup
Board Dimensions: 4x4 grid, a cyclic board where players move in a predefined path.
Cell Distribution: The board’s cells are organized to ensure a balanced mix of events, challenges, and points, as follows:
Cell ID
Name
Description
Count
ST
Start
Starting point for all players. No effect.
1
GP
Gain Points
Grants a random amount of points 
(range: 5-20).
2
LP
Lose Points
Subtracts a random amount of points 
(range: 5-20).
1
MG
Mini-Game
Initiates a random mini-game between two players.
3
RE
Random Event
Triggers one of the existing random events (excluding mini-games).
2
MF
Move Forward
Moves the player forward by a set number of cells (1-3).
2
MB
Move Backward
Moves the player backward by a set number of cells (1-3).
2
SK
Skip Turn
Causes the player to lose their next turn.
2
DE
Zero Points 
(Death Cell)
Resets the player’s score to zero.
1








3

(MF)

→
4

(MG)

↓
9

(MG)

→
10

(GP)

↓
2

(LP)

↑
5

(RE)

↓
8

(MB)

↑
11

(MF)

↓
1

(GP)

↑

6

(DE)

→
7

(LP)

↑
12

(MB)

↓
0

(ST)

↑
15

(MG)

←
14

(SK)

←
13

(RE)

←

















Mini-Games
Minigames allow players to compete against other players.
A random mini-game is selected when a player lands on a mini-game cell. The following mini-games are available:
Hot Potato
Objective: Avoid being the player holding the button when time runs out.
Rules: The control base generates a hidden countdown time. Players press the button to decrement time, not knowing the exact time remaining. The buzzer beeps faster as the timer approaches zero, and the player holding the button when it reaches zero loses the game.
It's reminiscent of the classic "Hot Potato" game.
Blind Timer
Objective: Estimate a target time without a clock.
Rules: The control base generates a target time (in seconds). Each player holds the button for the time they believe matches the target. The player closest to the target time wins.
This game tests players' internal sense of time.
Number Guesser
Objective: Guess a hidden target number without going over.
Rules: Players have to guess a number between a specified range, without knowing the control base’s hidden number. The player with the closest guess, without exceeding the target, wins the round.
It's reminiscent of the classic "The Price is Right" game ("Precio Justo" in Spanish).
Tug of War
Objective: Pull the virtual rope onto your screen.
Rules: Players repeatedly press the button to move a virtual rope closer to their side on the control base LCD screen. The first player to pull the rope past a threshold wins.
It's reminiscent of the classic "Tug of War" game ("Tira y Afloja" in Spanish).
Rock, Paper, Scissors 
Objective: Outplay the opponent in a classic Rock, Paper, Scissors match.
Rules: Both players select Rock, Paper, or Scissors. Standard Rock, Paper, Scissors rules determine the winner.
Last Stick Standing
Objective: Be the player who removes the last stick.
Rules: The game starts with a random number of virtual sticks (e.g., 10). Players take turns removing 1 or 2 sticks each. The player who removes the last stick wins the game.
The LCD screen displays the remaining sticks as lines.
It's reminiscent of the classic "Nim" game.
Quick Reflexes 
Objective: Be the fastest to react when the buzzer sounds.
Rules: Players wait for an audio cue from the control base. As soon as the buzzer goes off, players must press their buttons as quickly as possible. The first player to react and press the button wins the game. However, if a player presses too early, they’re disqualified from the round.

Here's a summary table of the mini-games:
Mini-Game
Objective
Rules
Hot Potato
Avoid holding the button when time runs out
Hidden countdown timer. Players press button to decrement time; buzzer beeps faster as timer nears zero.
Blind Timer
Estimate target time without a clock
Control base sets a target time (in seconds). Players hold button for time they believe matches the target.
Number Guesser
Guess a hidden number without going over
Players guess a number within a range. Closest guess without exceeding the target wins.
Tug of War
Pull the virtual rope onto your screen
Players repeatedly press button to pull a rope on LCD screen; first to pull past threshold wins.
Rock, Paper, Scissors
Outplay opponent in Rock, Paper, Scissors
Both players select Rock, Paper, or Scissors. Standard rules determine the winner.
Last Stick Standing
Be the player to remove the last stick
Random number of sticks; players take turns removing 1 or 2. Last to remove a stick wins.
Quick Reflexes
Fastest reaction to buzzer
Wait for buzzer; first to press button wins.


Turn Mechanics
The game follows a turn-based system, where players take turns moving their meeples along the board. The turn mechanics are as follows:
When a player's turn starts, their meeple LED lights up.
Then, the player rolls a random movement (1-6 squares) for their meeple.
The player moves their meeple cell by cell, following the path on the board. As they advance, the LCD screen displays the current cell's number and the number of movements remaining.
Moving through cells does not trigger their effects until the player lands on the final cell of their movement.
When the movement count reaches zero, the LCD screen displays the cell's event and its effects on the player.
If the player lands on a mini-game cell, a game is chosen randomly from the available mini-games and its name appears on the screen, and the game starts.
If the player lands on a win/lose points cell, or the zero points cell, the player's score is updated accordingly on the screen.
If the player lands on a move forward/backward cell, the counter is updated accordingly, and the player continues their turn, moving forwards or backwards the specified number of cells. The last cell they land on triggers its event.
If the player lands on a skip turn cell, the player loses their next turn.
If the player lands on a random event cell, a random event is chosen from the available events, and its name appears on the screen, following the mechanics described above.
Different sounds and animations accompany each event, mini-game, or action, enhancing the player's experience.


### Mini-Game Mechanics

#### MQTT Communication

To facilitate communication between the game controller and control bases, an enum `MinigameType` is defined, containing all the mini-games available in the game. Both the game controller and control bases share the same enum definition in their code.

```python
class MinigameType(enum.Enum):
    HotPotato = 1
    BlindTimer = 2
    NumberGuesser = 3
    TugOfWar = 4
    RockPaperScissors = 5
    LastStickStanding = 6
    QuickReflexes = 7
```

When a meeple lands on a mini-game cell, the controller selects a random mini-game from the `MinigameType` enum and publishes the chosen mini-game to MQTT. The topic used is `game/minigame`, and the payload is the chosen mini-game's enum value. For example, for `NumberGuesser`, the controller publishes to `game/minigame` with the payload `3`.

#### Game Controller Logic

The game controller uses an interface `MiniGameInterface` that defines the methods all mini-games must implement. The controller maintains a global variable `currentMiniGame` to hold the current mini-game instance.

When a mini-game is selected, the controller creates an instance of the chosen mini-game and assigns it to `currentMiniGame`. It then publishes a message to MQTT to notify the control bases that a mini-game has started.

```python
class MiniGameInterface:
    def handleMQTTMessage(self, topic: str, payload: str) -> None:
        pass
```

When the game controller receives a message, the `on_message` method checks the game state and calls the appropriate function to handle the message. In the `MINIGAME` state, the controller calls the `handleMQTTMessage` method of the current mini-game. This is why all mini-games must implement this method.

Each mini-game class will have its own logic to handle the specific game flow, ensuring that the game mechanics are executed correctly.

---
NumberGuesser
Game Controller flow:
1. After the game has been decided and published to MQTT, the game controller enters a f



