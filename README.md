# Plugin: Modular Features Extra Actions

## The plugin:

Inspired by some features from UE5's Valley of the Ancient sample, this plugin integrates GAS and Enhanced Input to the existing Game Features and Modular plugins from Unreal Engine, extending some Feature Actions to:

1. Add Attribute;
2. Add Gameplay Abilities;
3. Add Gameplay Effects;
4. Add Enhanced Input Mappings;
5. Spawn Actors;


**This project is being built on Unreal Engine 5 Preview 1**

Post on Unreal Engine Forum: https://forums.unrealengine.com/t/free-modularfeatures-extraactions-plugin-modular-gas-enhanced-input-and-more/495400

# Installation
Just clone this repository or download the .zip file in the latest version and drop the files inside the 'Plugins/' folder of your project folder.
Note that if your project doesn't have a 'Plugins' folder, you can create one.

## Essentials informations:

To bind a ability input, implement the **IAbilityInputBinding** interface inside your pawn or controller.

![ImplementingInterface](https://user-images.githubusercontent.com/77353979/152875609-16b18e23-3b54-4e3e-9d5b-105be27cad08.png)

And implement **SetupAbilityInput** and **RemoveAbilityInput** functions _(I'm using a client RPC due to multiplayer replication)_

![AbilityInput](https://user-images.githubusercontent.com/77353979/152875174-483a0105-7f6e-43b6-8d3a-7a75dc9fe597.png)


## More infos on the way...

See the official UE documentation for Modular Game Features: https://docs.unrealengine.com/5.0/en-US/GameplayFeatures/GameFeaturesAndModularGameplay/

See the official UE documentation for Enhanced Input: https://docs.unrealengine.com/5.0/en-US/GameplayFeatures/EnhancedInput/

See the official UE documentation for Gameplay Ability System: https://docs.unrealengine.com/4.27/en-US/InteractiveExperiences/GameplayAbilitySystem/

Take a look at UE5's Valley of the Ancient sample: https://docs.unrealengine.com/5.0/en-US/ContentAndSamples/ValleyOfTheAncient/
