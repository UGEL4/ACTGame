# ALS Skeletons
A repo for housing ALS-Refactored skeletons, animations, and ABT instances for various skeletons in the UE ecosystem, such as the UE5 mannequins.

Everything is 99% automated. Adding new humanoid skeletons should typically be a breeze, with only a few manual steps.

The UE5 assets in this repo are a demonstration of this process. Everything was retargeted from the source ALS animations, AnimationBlueprint Templates (ABTs) instanced for the UE5 skeleton, data assets copied and updated to hold the retargeted assets, control rig, example BP, and modular ABP, all inheriting from the ALS base classes to ensure upstream changes propagate down.

## Why?
This plugin makes it trivial to get started with ALS-Refactored (which is hands-down the best ALS implementation out there) using the UE5 mannequns, and the automation pattern makes it trivial to add any other humanoid skeletons. All of the core animation logic is now encapsulated in AnimationBlueprint Templates in upstream ALS-Refactored, which means that we instance the ABTs, and automatically inherit any logic changes or adjustments made upstream. The only change we make to the ABT instances per skeleton is to use the retargeted animation sequences, montages, blendspaces, and data assets, which is fully automated.

## Install Plugin
Clone the repository to your project's `Plugins` folder. Use the default repo name only: `ALS-Skeletons`.

This project depends upon [ALS-Refactored](https://github.com/Sixze/ALS-Refactored). Be sure to follow the setup instructions there.

## Add a New Skeleton
### 0. Add New Assets to Repo
Ensure the target skeleton and skeletal mesh have been added to this repo in a logical location, following the example of the UE5 mannequins. Skip this step and just keep your skeleton in your project if you don't plan on contributing it here.

**CRITICALLY:** be sure to run the `Setup Als Skeleton` on your new skeleton first. Right click your skeleton (not skeletal mesh, skeleton), `Scripted Asset Actions` -> `Setup Als Skeleton`. In the window, ensure all options are selected, and then click `Ok`. This will ensure your skeleton will work with the example ALS setup.

### 1. Run Automation
Run the alsretarget script for the target skeleton. 

- In the editor, at the very bottom next to the Content Drawer and Output Log, change Cmd mode to Python (REPL).
- Run `import alsretarget`
- Then run:
```python
alsretarget.retarget(
  outputDir="/ALS-Skeletons/ALSTest",
  targetSkel="/ALS-Skeletons/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin",
  targetSkelIkRtg="/ALS-Skeletons/Characters/Mannequins/Rigs/RTG_Als_UE5.RTG_Als_UE5",
  targetSkelMesh="/ALS-Skeletons/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple",
  alsNameReplace="UE5",
)
```
The above is the exact command used to produce the UE5 results in this repo. The parameters are self-explanatory, review the python script in this repo at /Content/Python/alsretarget.py for clarification.

**IMPORTANT:** when prompted to fixup / remove redirectors, be sure to accept, else you will leave a bunch of redirectors in the `ALS-Refactored` repo, which you shouldn't do.

This will perform lots of retargeting operations, saving, and will take a minute or so. Be patient. IT IS NOT FROZEN. You'll know the script is finished by keeping an eye on the Output Log, no output is emitted until the script finishes.

### 2. CR
The retargeting automation will produce a new CR for your skeleton at: `{outputDir}/CR_{alsNameReplace}`. It will be ready to use immediately. However, depending on your skeleton and bone names, you will have to watch out for the following:
- Update the Spine Rotation Bones in Class Defaults -> Settings to match the bones of the new skeleton.
- Spot check the CR's referenced foot bone and hand bones, and any other refrerenced bones in the CR logic. Update as needed.

### 3. ABP
The retargeting automation will produce a new ABP for your sekeleton at: `{outputDir}/AB_{alsNameReplace}`. It IS NOT ready for immediate use, and will always require some manual updates. Other than the internal structure of the animation graph itself, everything else is good to go and is taken care of automatically by the automation suite.
- Replicate the structure of AB_Als, using your retargeted skeleton specific linked animation graphs. You can literally copy/paste the graph sctructure from ALS's on AB_Als, but after pasting, go to each node and select your retargeted version of the corresponding asset. It will have the same name, but with your `{alsNameReplace}` instead of `_Als_`.
- Be sure to open the ABP, and open the `Overlay` linked layer, update it to have a single `Local Space Ref Pose` feeding to the output pose. That's all.

### 4. BP
The retargeting automation will produce a new character BP for your skeleton at: `{outputDir}/B_{alsNameReplace}_Character`. It will be ready to use immediately, and should not require any changes or updates. Remember that it is an EXAMPLE CHARACTER. Its only purpose is as an example. It is NOT recommended to use as a foundation for your project's character.

### 5. Profit!
Now just update the Playground's GameMode to use your new BP_XXX_Character as the default character, and you now have ALS working with your new skeleton. The entire animation stack, and the example character are subclasses, so upstream changes will propagate down, greatly reducing maintenance burden.
