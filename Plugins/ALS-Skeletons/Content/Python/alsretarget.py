"""
alsretarget.py

Example:
```
import alsretarget
alsretarget.retarget(
    outputDir="/ALS-Skeletons/Characters/UE5/ALS",
    targetSkel="/ALS-Skeletons/Characters/UE5/Meshes/SK_Mannequin.SK_Mannequin",
    targetSkelIkRtg="/ALS-Skeletons/Characters/UE5/Rigs/RTG_Als_UE5.RTG_Als_UE5",
    targetSkelMesh="/ALS-Skeletons/Characters/UE5/Meshes/SKM_Manny_Simple.SKM_Manny_Simple",
    alsNameReplace="UE5",
)
```

Params:
    outputDir - directory all retargeted/instanced/copied output is placed under.
    targetSkel - Skeleton asset path for the new character.
    targetSkelIkRtg - IK Retargeter asset path, source ALS skeleton -> targetSkel.
    targetSkelMesh - SkeletalMesh asset path, used for the Control Rig preview mesh, the character's
        Mesh component, and to refresh the copied Control Rig's hierarchy.
    alsNameReplace - token replacing "Als" throughout every output asset name; no leading/trailing "_".
"""

import re
import unreal


# ---------------------------------------------------------------------------
# Config - project-fixed. Edit these for your project.
# ---------------------------------------------------------------------------

ABT_DIR = "/ALS/ALS/Character/AnimationInstances"
ANIMS_ROOT = "/ALS/ALS/Animations"
ALS_ROOT = "/ALS"

# Variables in instances ABTs to replace with retargeted assets.
ABT_VARIABLE_NAMES = {
    "ABT_Als_Grounded": ["AnimRoll", "AnimStandToCrouch", "AnimCrouchToStand"],
    "ABT_Als_Head": ["BlendSpaceLook"],
    "ABT_Als_Layering": ["AnimStand", "AnimCrouch"],
    "ABT_Als_Locomotion": [
        "AnimLandLight", "AnimLandHeavy", "AnimLandLightAdditive", "AnimLandHeavyAdditive",
        "BlendSpaceAirLean", "AnimJumpWalkLeft", "AnimJumpRunLeft", "AnimJumpWalkRight",
        "AnimJumpRunRight", "AnimJumpLoop", "AnimFlail", "AnimFall", "AnimFallFast"
    ],
    "ABT_Als_Ragdolling": ["AnimFlail"],
    "ABT_Als_Standing": [
        "AnimStopLeft", "AnimStopRight", "BlendSpaceWalkRunForward", "BlendSpaceWalkRunBackward", "BlendSpaceWalkRunLeftForward", "BlendSpaceWalkRunLeftBackward",
        "BlendSpaceWalkRunRightForward", "BlendSpaceWalkRunRightBackward", "BlendSpaceLean", "AnimSprint", "AnimSprintAcceleration", "AnimAccelerationForward",
        "AnimAccelerationBackward", "AnimAccelerationLeft", "AnimAccelerationRight", "AnimStandPose", "AnimRotate90Left", "AnimRotate90Right",
    ],
    "ABT_Als_Crouching": [
        "AnimCrouchWalkForward", "AnimCrouchWalkBackward", "AnimCrouchWalkLeftForward", "AnimCrouchWalkLeftBackward", "AnimCrouchWalkRightForward",
        "AnimCrouchWalkRightBackward", "AnimCrouchWalkPose", "AnimCrouchPose", "AnimCrouchRotate90Left", "AnimCrouchRotate90Right", "BlendSpaceLean",
    ],
    "ABT_Als_Barrel": ["AnimOverlayPoses", "AnimIdle"],
    "ABT_Als_Binoculars": ["AnimOverlayPoses", "AnimIdle", "AnimOverlayAim", "AnimOverlayAimCrouch"],
    "ABT_Als_Bow": ["AnimOverlayPoses", "AnimIdle", "AnimOverlayAim", "AnimOverlayAimCrouch"],
    "ABT_Als_Box": ["AnimOverlayPoses", "AnimIdle"],
    "ABT_Als_Default": ["AnimOverlayPoses", "AnimIdle"],
    "ABT_Als_Feminine": ["AnimOverlayPoses", "AnimIdle"],
    "ABT_Als_HandsTied": ["AnimOverlayPoses", "AnimIdle"],
    "ABT_Als_Injured": ["AnimOverlayPoses", "AnimIdle"],
    "ABT_Als_Masculine": ["AnimOverlayPoses", "AnimIdle"],
    "ABT_Als_PistolOneHanded": ["AnimOverlayPoses", "AnimIdle", "AnimOverlayAim", "AnimOverlayAimCrouch"],
    "ABT_Als_PistolTwoHanded": ["AnimOverlayPoses", "AnimIdle", "AnimOverlayAim", "AnimOverlayAimCrouch"],
    "ABT_Als_Rifle": ["AnimOverlayPoses", "AnimIdle", "AnimOverlayAim", "AnimOverlayAimCrouch", "AnimRunArms", "AnimSprintArms", "AnimSprintImpulseArms"],
    "ABT_Als_Torch": ["AnimOverlayPoses", "AnimIdle", "AnimOverlayAim", "AnimOverlayAimCrouch"],
}

ABT_TEMPLATE_NAME_PREFIX = "ABT_"
ABT_INSTANCE_NAME_PREFIX = "AB_"

ANIM_ASSET_VARIABLE_TYPES = (
    unreal.AnimSequence,
    unreal.AnimMontage,
    unreal.BlendSpace,
    unreal.BlendSpace1D,
)

ADDITIVE_BASE_POSE_TYPES_WITH_ASSET_REF = (
    unreal.AdditiveBasePoseType.ABPT_ANIM_SCALED,
    unreal.AdditiveBasePoseType.ABPT_ANIM_FRAME,
    unreal.AdditiveBasePoseType.ABPT_LOCAL_ANIM_FRAME,
)

DATA_ASSETS_ROOT = "/ALS/ALS/Data"
DATA_ASSETS_TO_RETARGET = [
    "/ALS/ALS/Data/AnimationInstance/AIS_Als_Default.AIS_Als_Default",
    "/ALS/ALS/Data/Character/CS_Als_Default.CS_Als_Default",
    "/ALS/ALS/Data/Character/Mantle/MS_Als_High.MS_Als_High",
    "/ALS/ALS/Data/Character/Mantle/MS_Als_High_InAir.MS_Als_High_InAir",
    "/ALS/ALS/Data/Character/Mantle/MS_Als_Low.MS_Als_Low",
    "/ALS/ALS/Data/Character/Mantle/MS_Als_Low_BothHands.MS_Als_Low_BothHands",
    "/ALS/ALS/Data/Character/Mantle/MS_Als_Low_Box.MS_Als_Low_Box",
    "/ALS/ALS/Data/Character/Mantle/MS_Als_Low_Left.MS_Als_Low_Left",
    "/ALS/ALS/Data/Character/Mantle/MS_Als_Low_Right.MS_Als_Low_Right",
]
AIS_DEFAULT_PATH = DATA_ASSETS_TO_RETARGET[0]  # main ABP's Settings
CS_DEFAULT_PATH = DATA_ASSETS_TO_RETARGET[1]   # character subclass's Settings

MAIN_ANIM_INST_CLASS = "ALS.AlsAnimationInstance"
ALI_OVERLAY_INTERFACE_PATH = "/ALS/ALS/Character/ALI_Overlay.ALI_Overlay"

ALS_CHARACTER_BP_PATH = "/ALS/ALS/Character/B_Als_Character.B_Als_Character"
MANTLE_SETTINGS_VARS = ["MSHigh", "MSHighAir", "MSLow", "MSLowBothHands", "MSLowLeft", "MSLowRight", "MSLowBox"]
OVERLAY_MAP_VAR = "OverlayAnimationInstanceClasses"
DEFAULT_OVERLAY_VAR = "DefaultOverlay"
SETTINGS_VAR = "Settings"
CHARACTER_MESH_VAR = "Mesh"

ALS_CONTROL_RIG_PATH = "/ALS/ALS/Character/CR_Als.CR_Als"

# Redirectors we do not own and must not fix up, by asset name.
REDIRECTORS_TO_KEEP = ("AB_Als_View",)

_LOG_PREFIX = "[alsretarget]"

_ALS_LIB = getattr(unreal, "AlsRetargetLibrary", None)
if _ALS_LIB is None:
    raise RuntimeError(f"{_LOG_PREFIX} unreal.AlsRetargetLibrary is required - compile AlsRetarget.h into an editor module first")

_asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
_asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
_asset_lib = unreal.EditorAssetLibrary
_editor_asset_subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)

# {source animation asset: retargeted asset}, the object-keyed form the C++ helper takes.
_REPLACEMENT_OBJECTS = {}

# {source Data Asset object: retargeted copy}, populated in stage 3.
_DATA_ASSET_REPLACEMENTS = {}

_DUPLICATE_SUFFIX_RE = re.compile(r"\d+$")
_REDIRECTOR_CLASS_NAME = "ObjectRedirector"

_ALS_SEARCH_LONG = "_Als_"
_ALS_SEARCH_SHORT = "_Als"


# ---------------------------------------------------------------------------
# Path helpers
# ---------------------------------------------------------------------------

def _norm(path):
    """Strips trailing slashes from a path."""
    return path.rstrip("/")


def _to_asset_path(object_path):
    """Strips the object/class extension from an asset path."""
    return object_path.split(".")[0]


def _package_of(object_path):
    """Returns the containing package path of an asset."""
    return _to_asset_path(object_path).rsplit("/", 1)[0]


def _name_of(package_path):
    """Returns the asset name from a package path."""
    return package_path.rsplit("/", 1)[-1]


def _mirror_path(source_root, dest_root, source_package_path):
    """Re-roots a package path from source_root onto dest_root preserving sub-folders."""
    source_root = _norm(source_root)
    dest_root = _norm(dest_root)
    if source_package_path != source_root and not source_package_path.startswith(source_root + "/"):
        raise ValueError(f"'{source_package_path}' is not located under expected root '{source_root}'")
    return dest_root + source_package_path[len(source_root):]


def _case_insensitive_replace(text, search, replace):
    """Predicts an asset's renamed result via case-insensitive regex swap."""
    if not search:
        return text
    return re.sub(re.escape(search), replace, text, flags=re.IGNORECASE)


def _is_retarget_output(name, search, replace):
    """True when a name already looks like the product of an earlier run."""
    if not search or not replace:
        return False
    lowered = name.lower()
    return replace.lower() in lowered and search.lower() not in lowered


def _ensure_dir(package_path):
    """Creates a directory path if it does not already exist."""
    if not _asset_lib.does_directory_exist(package_path):
        _asset_lib.make_directory(package_path)


def _normalize_als_name_replace(als_name_replace):
    """Strips leading/trailing '_' from alsNameReplace, warning if any were present."""
    trimmed = als_name_replace.strip("_")
    if trimmed != als_name_replace:
        unreal.log_warning(f"{_LOG_PREFIX} alsNameReplace '{als_name_replace}' should not start/end with '_' - using '{trimmed}'")
    if not trimmed:
        unreal.log_error(f"{_LOG_PREFIX} alsNameReplace is empty after trimming '_'")
    return trimmed


# ---------------------------------------------------------------------------
# Editor property resolution
# ---------------------------------------------------------------------------

_SNAKE_BOUNDARY_ACRONYM = re.compile(r"(.)([A-Z][a-z]+)")
_SNAKE_BOUNDARY_SIMPLE = re.compile(r"([a-z0-9])([A-Z])")


def _snake_case(name):
    """Converts a UE PascalCase property name to its Python alias, e.g. MSHighAir -> ms_high_air."""
    stepped = _SNAKE_BOUNDARY_ACRONYM.sub(r"\1_\2", name)
    return _SNAKE_BOUNDARY_SIMPLE.sub(r"\1_\2", stepped).lower()


def _property_name_candidates(name):
    """Returns the UE and Python spellings to try for a property name."""
    snake = _snake_case(name)
    return [name] if snake == name else [name, snake]


def _list_readable_properties(obj):
    """Best-effort list of every editor property name readable off obj."""
    names = set()
    for cls in type(obj).__mro__:
        for attr, value in vars(cls).items():
            if attr.startswith("_"):
                continue
            if isinstance(value, property) or type(value).__name__.endswith("descriptor"):
                names.add(attr)
    return sorted(names)


def _log_readable_properties(obj, label):
    """Dumps every readable property name on obj, six per line."""
    available = _list_readable_properties(obj)
    unreal.log_error(f"{_LOG_PREFIX} {len(available)} readable properties on {label}:")
    for index in range(0, len(available), 6):
        unreal.log_error(f"{_LOG_PREFIX}   " + ", ".join(available[index:index + 6]))


def _resolve_property_name(obj, name, label):
    """Returns the spelling of name that obj answers to; raises with a property dump when there is none."""
    for candidate in _property_name_candidates(name):
        try:
            obj.get_editor_property(candidate)
            return candidate
        except Exception:
            continue

    unreal.log_error(f"{_LOG_PREFIX} Could not resolve property '{name}' on {label}.")
    _log_readable_properties(obj, label)
    raise RuntimeError(f"{_LOG_PREFIX} Property '{name}' not found on {label} - see the log for the available properties")


def _get_property(obj, name, label):
    """Reads an editor property by either of its spellings."""
    return obj.get_editor_property(_resolve_property_name(obj, name, label))


def _set_property(obj, name, value, label):
    """Writes an editor property by either of its spellings."""
    obj.set_editor_property(_resolve_property_name(obj, name, label), value)


def _try_set_property(obj, names, value, label):
    """Writes the first of names that obj answers to; warns rather than raising on a miss."""
    if isinstance(names, str):
        names = [names]

    for name in names:
        for candidate in _property_name_candidates(name):
            try:
                obj.get_editor_property(candidate)
                obj.set_editor_property(candidate, value)
                return candidate
            except Exception:
                continue

    unreal.log_warning(f"{_LOG_PREFIX} Could not set any of {names} on {label} - skipping.")
    _log_readable_properties(obj, label)
    return None


# ---------------------------------------------------------------------------
# Commit + move plumbing
# ---------------------------------------------------------------------------

def _save_dirty_packages():
    """Saves every dirty content package in a single batched call."""
    packages = list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    if packages:
        unreal.EditorLoadingAndSavingUtils.save_packages(packages, only_dirty=True)
    return len(packages)


def _commit(label, scan_paths):
    """Saves every dirty package in one batch and refreshes the asset registry."""
    unreal.log(f"{_LOG_PREFIX} Commit: saved {_save_dirty_packages()} packages after {label}.")

    paths = []
    for path in scan_paths:
        path = _norm(path)
        if path and path not in paths:
            _ensure_dir(path)
            paths.append(path)

    if paths:
        _asset_registry.scan_paths_synchronous(paths, force_rescan=True)
    _asset_registry.wait_for_completion()


def _asset_class_name(asset_data):
    """Returns the class name recorded on an FAssetData."""
    return str(asset_data.asset_class_path.asset_name)


def _is_redirector_at(asset_path):
    """True when the asset registered at asset_path is an ObjectRedirector."""
    asset_data = _editor_asset_subsystem.find_asset_data(asset_path)
    if asset_data is None or str(asset_data.package_name) in ("", "None"):
        return False
    return _asset_class_name(asset_data) == _REDIRECTOR_CLASS_NAME


def _move_asset(obj, dest_path):
    """Moves an asset to dest_path, leaving a redirector behind at its previous location."""
    source_path = _to_asset_path(obj.get_path_name())
    if source_path == dest_path:
        return obj

    _ensure_dir(_package_of(dest_path))

    # A redirector sitting on the destination name blocks the move.
    if _is_redirector_at(dest_path):
        _asset_lib.delete_asset(dest_path)

    if _asset_lib.does_asset_exist(dest_path):
        existing_obj = unreal.load_asset(dest_path)
        if existing_obj and existing_obj != obj:
            _asset_lib.consolidate_assets(obj, [existing_obj])
        if _is_redirector_at(dest_path):
            _asset_lib.delete_asset(dest_path)

    moved_obj = _ALS_LIB.move_asset_leaving_redirector(obj, _package_of(dest_path), _name_of(dest_path))
    if moved_obj is None:
        unreal.log_error(f"{_LOG_PREFIX} Failed to move '{source_path}' -> '{dest_path}'")
        return obj

    return moved_obj


# ---------------------------------------------------------------------------
# Asset introspection
# ---------------------------------------------------------------------------

def _anim_assets_under(package_path):
    """Yields every animation asset under a directory."""
    for asset_data in _asset_registry.get_assets_by_path(_norm(package_path), recursive=True):
        obj = asset_data.get_asset()
        if isinstance(obj, ANIM_ASSET_VARIABLE_TYPES):
            yield obj


def _abt_instance_name(template_name, als_name_replace):
    """Derives an ABT instance's target name, e.g. ABT_Als_Rifle -> AB_{alsNameReplace}_Rifle."""
    template_stem = ABT_TEMPLATE_NAME_PREFIX.rstrip("_")
    instance_stem = ABT_INSTANCE_NAME_PREFIX.rstrip("_")

    name = template_name
    if name.startswith(template_stem):
        name = instance_stem + name[len(template_stem):]

    return _case_insensitive_replace(name, _ALS_SEARCH_LONG, f"_{als_name_replace}_")


def _find_template_abts(abt_dir):
    """Recursively finds Template Animation Blueprints under a directory."""
    templates = []
    for asset_data in _asset_registry.get_assets_by_path(_norm(abt_dir), recursive=True):
        obj = asset_data.get_asset()
        if isinstance(obj, unreal.AnimBlueprint) and obj.get_editor_property("target_skeleton") is None:
            templates.append(obj)
    return templates


def _instance_abt_for_skeleton(template_abt, target_skeleton, dest_package_path, dest_asset_name):
    """Creates a concrete AnimBlueprint instance from a template."""
    _ensure_dir(dest_package_path)
    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property("parent_class", template_abt.generated_class())
    factory.set_editor_property("target_skeleton", target_skeleton)
    factory.set_editor_property("template", False)

    new_abt = _asset_tools.create_asset(
        asset_name=dest_asset_name, package_path=dest_package_path, asset_class=None, factory=factory
    )
    unreal.BlueprintEditorLibrary.compile_blueprint(new_abt)
    return new_abt


def _get_anim_asset_variables(anim_blueprint):
    """Gets animation assets explicitly declared in ABT_VARIABLE_NAMES."""
    cdo = unreal.get_default_object(anim_blueprint.generated_class())
    found = {}
    for name in ABT_VARIABLE_NAMES.get(anim_blueprint.get_name(), []):
        try:
            val = cdo.get_editor_property(name)
        except Exception:
            continue
        if isinstance(val, ANIM_ASSET_VARIABLE_TYPES):
            found[name] = val
    return found


def _get_additive_reference(anim_asset):
    """Returns the base pose asset reference if the animation is additive, else None."""
    if isinstance(anim_asset, unreal.AnimSequence):
        if anim_asset.get_editor_property("additive_anim_type") == unreal.AdditiveAnimationType.AAT_NONE:
            return None
        if anim_asset.get_editor_property("ref_pose_type") not in ADDITIVE_BASE_POSE_TYPES_WITH_ASSET_REF:
            return None
        ref_seq = anim_asset.get_editor_property("ref_pose_seq")
        return None if (ref_seq is None or ref_seq.get_path_name() == anim_asset.get_path_name()) else ref_seq

    if isinstance(anim_asset, unreal.AnimMontage):
        preview_base = anim_asset.get_editor_property("preview_base_pose")
        return None if (preview_base is None or preview_base.get_path_name() == anim_asset.get_path_name()) else preview_base
    return None


def _set_additive_reference(anim_asset, new_reference):
    """Updates the base pose asset reference for an additive animation."""
    if isinstance(anim_asset, unreal.AnimSequence):
        anim_asset.set_editor_property("ref_pose_seq", new_reference)
    elif isinstance(anim_asset, unreal.AnimMontage):
        anim_asset.set_editor_property("preview_base_pose", new_reference)


def _build_replacement_objects(retarget_map):
    """Rebuilds the object-keyed {source asset: retargeted asset} map the C++ helper takes."""
    _REPLACEMENT_OBJECTS.clear()
    for source_path, retargeted in retarget_map.items():
        if not retargeted:
            continue
        source_obj = unreal.load_asset(_to_asset_path(source_path))
        if source_obj:
            _REPLACEMENT_OBJECTS[source_obj] = retargeted
    return _REPLACEMENT_OBJECTS


def _replace_references(obj):
    """Repoints every reference obj holds on a source animation to its retargeted counterpart."""
    return _ALS_LIB.replace_references(obj, _REPLACEMENT_OBJECTS)


def _repair_animation_assets(assets, retarget_map):
    """Repoints stale source references across the retargeted set and re-links the montages."""
    targets = [asset for asset in assets if asset]
    if not targets:
        return

    _build_replacement_objects(retarget_map)
    replaced = _ALS_LIB.repair_animation_assets(targets, _REPLACEMENT_OBJECTS)
    unreal.log(f"{_LOG_PREFIX} Repaired {len(targets)} animation assets, {replaced} references repointed off the original animations.")


# ---------------------------------------------------------------------------
# Stage 1 - retarget animations in place
# ---------------------------------------------------------------------------

def _report_retarget_output(created_asset_datas, anims_root):
    """Logs what the batch produced, flagging anything that landed outside the animations root."""
    if not created_asset_datas:
        unreal.log_warning(f"{_LOG_PREFIX} Batch retarget reported no output assets.")
        return

    root_prefix = _norm(anims_root) + "/"
    outside = sorted({str(asset_data.package_name) for asset_data in created_asset_datas
                      if not str(asset_data.package_name).startswith(root_prefix)})

    unreal.log(f"{_LOG_PREFIX} Batch retarget reported {len(created_asset_datas)} output assets.")
    for package_name in outside:
        unreal.log_warning(f"{_LOG_PREFIX}   '{package_name}' is outside '{anims_root}' - will not be moved to the output directory")


def _stage_retarget_animations(anims_root, ik_retargeter, search, replace):
    """Retargets every animation asset under anims_root, in place, overwriting prior output."""
    expected_names = {}
    source_asset_datas = []
    already_retargeted = 0
    would_clobber_source = []

    for obj in _anim_assets_under(anims_root):
        name = obj.get_name()
        if _is_retarget_output(name, search, replace):
            already_retargeted += 1
            continue

        retargeted_name = _case_insensitive_replace(name, search, replace)
        if retargeted_name == name:
            would_clobber_source.append(obj.get_path_name())
            continue

        source_path = obj.get_path_name()
        expected_names[source_path] = retargeted_name
        source_asset_datas.append(_editor_asset_subsystem.find_asset_data(_to_asset_path(source_path)))

    unreal.log(
        f"{_LOG_PREFIX} Stage 1: retargeting {len(source_asset_datas)} animation assets under '{anims_root}' "
        f"({already_retargeted} already retargeted, {len(would_clobber_source)} skipped)..."
    )
    for path in would_clobber_source:
        unreal.log_warning(f"{_LOG_PREFIX}   skipped '{path}' - name has no '{search}' to replace")

    if source_asset_datas:
        created = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
            source_asset_datas, None, None, ik_retargeter,
            search=search, replace=replace,
            use_source_path=True, include_referenced_assets=True,
            overwrite_existing_files=True
        )
        _report_retarget_output(created, anims_root)

    return expected_names


# ---------------------------------------------------------------------------
# Stage 2 - deduplicate the retarget output
# ---------------------------------------------------------------------------

def _stage_deduplicate_animations(anims_root, expected_names):
    """Folds any stray numbered copies into their canonical asset and maps sources onto results."""
    canonical = {expected_name: None for expected_name in expected_names.values()}

    duplicates = []
    for obj in _anim_assets_under(anims_root):
        if obj.get_path_name() in expected_names:
            continue
        if obj.get_name() in canonical:
            canonical[obj.get_name()] = obj
        else:
            duplicates.append(obj)

    duplicates.sort(key=lambda asset: asset.get_name())  # "Name1" before "Name2" - first copy becomes canonical

    unreal.log(
        f"{_LOG_PREFIX} Stage 2: {sum(1 for a in canonical.values() if a)} canonical assets, "
        f"{len(duplicates)} stray copies to fold in..."
    )

    for dup in duplicates:
        base_name = _DUPLICATE_SUFFIX_RE.sub("", dup.get_name()).rstrip("_")
        target = canonical.get(base_name)
        if target is None:
            unreal.log_error(f"{_LOG_PREFIX} Cannot fold '{dup.get_path_name()}' - no canonical asset named '{base_name}'")
            continue

        unreal.log_warning(f"{_LOG_PREFIX} Folding leftover copy '{dup.get_path_name()}' into '{base_name}'")
        _asset_lib.consolidate_assets(target, [dup])

    retarget_map = {}
    for source_path, expected_name in expected_names.items():
        final_asset = canonical.get(expected_name)
        if final_asset:
            retarget_map[source_path] = final_asset
        else:
            unreal.log_warning(f"{_LOG_PREFIX} No retargeted counterpart for '{source_path}' (expected '{expected_name}')")

    _repair_animation_assets(canonical.values(), retarget_map)

    for canonical_asset in canonical.values():
        if not canonical_asset:
            continue
        additive_ref = _get_additive_reference(canonical_asset)
        if additive_ref is not None and additive_ref.get_path_name() in retarget_map:
            _set_additive_reference(canonical_asset, retarget_map[additive_ref.get_path_name()])

    return retarget_map


# ---------------------------------------------------------------------------
# Stage 3 - ABTs and Data Assets
# ---------------------------------------------------------------------------

def _stage_process_data_assets(data_root, data_rtg_root, als_name_replace):
    """Duplicates each Data Asset beside its original and repoints its animation references."""
    pending_moves = []
    _DATA_ASSET_REPLACEMENTS.clear()

    for data_asset_path in DATA_ASSETS_TO_RETARGET:
        data_asset = unreal.load_asset(data_asset_path)
        if not data_asset:
            unreal.log_warning(f"{_LOG_PREFIX} Data Asset not found: '{data_asset_path}'")
            continue

        source_package = _package_of(data_asset_path)
        dest_package = _mirror_path(data_root, data_rtg_root, source_package)
        dest_name = _case_insensitive_replace(data_asset.get_name(), _ALS_SEARCH_LONG, f"_{als_name_replace}_")

        build_path = f"{source_package}/{dest_name}"
        if _asset_lib.does_asset_exist(build_path):
            _asset_lib.delete_asset(build_path)

        copied_data_asset = _asset_lib.duplicate_asset(_to_asset_path(data_asset_path), build_path)
        if not copied_data_asset:
            unreal.log_error(f"{_LOG_PREFIX} Failed to duplicate '{data_asset_path}' -> '{build_path}'")
            continue

        _replace_references(copied_data_asset)
        pending_moves.append((copied_data_asset, f"{dest_package}/{dest_name}"))
        _DATA_ASSET_REPLACEMENTS[data_asset] = copied_data_asset

    unreal.log(f"{_LOG_PREFIX} Stage 3: prepared {len(pending_moves)} Data Assets.")
    return pending_moves


def _stage_process_abts(abt_dir, abt_instance_dir, target_skeleton, retarget_map, als_name_replace):
    """Instances each template ABT beside its template and points its variables at the retargets."""
    pending_moves = []
    abt_instance_by_template = {}
    template_abts = _find_template_abts(abt_dir)

    for template_abt in template_abts:
        template_package = _package_of(template_abt.get_path_name())
        instance_package = _mirror_path(abt_dir, abt_instance_dir, template_package)
        instance_name = _abt_instance_name(template_abt.get_name(), als_name_replace)

        build_name = instance_name + "_TempRtg"
        build_path = f"{template_package}/{build_name}"
        if _asset_lib.does_asset_exist(build_path):
            _asset_lib.delete_asset(build_path)

        instance_abt = _instance_abt_for_skeleton(template_abt, target_skeleton, template_package, build_name)
        if not instance_abt:
            unreal.log_error(f"{_LOG_PREFIX} Failed to instance ABT from '{template_abt.get_path_name()}'")
            continue

        instance_cdo = unreal.get_default_object(instance_abt.generated_class())
        for var_name, source_anim in _get_anim_asset_variables(template_abt).items():
            source_path = source_anim.get_path_name()
            if source_path in retarget_map:
                instance_cdo.set_editor_property(var_name, retarget_map[source_path])

        # Catches anything ABT_VARIABLE_NAMES misses.
        _ALS_LIB.replace_references(instance_cdo, _REPLACEMENT_OBJECTS)

        unreal.BlueprintEditorLibrary.compile_blueprint(instance_abt)
        pending_moves.append((instance_abt, f"{instance_package}/{instance_name}"))
        abt_instance_by_template[template_abt.get_name()] = instance_abt

    unreal.log(f"{_LOG_PREFIX} Stage 3: prepared {len(pending_moves)} ABT instances from {len(template_abts)} templates.")
    return pending_moves, abt_instance_by_template


# ---------------------------------------------------------------------------
# Stage 4 - move everything to the output directory
# ---------------------------------------------------------------------------

def _stage_move_to_output(retarget_map, anims_root, anims_rtg_root, pending_moves):
    """Moves the retargeted animations, Data Assets and ABTs into the output directory."""
    moves = []
    for source_path, canonical_obj in retarget_map.items():
        if not canonical_obj:
            continue
        dest_package = _mirror_path(anims_root, anims_rtg_root, _package_of(source_path))
        moves.append((source_path, canonical_obj, f"{dest_package}/{canonical_obj.get_name()}"))

    unreal.log(f"{_LOG_PREFIX} Stage 4: moving {len(moves)} animations and {len(pending_moves)} ABT/Data Assets...")

    moved = 0
    for source_path, canonical_obj, dest_path in moves:
        result = _move_asset(canonical_obj, dest_path)
        retarget_map[source_path] = result
        if _to_asset_path(result.get_path_name()) == dest_path:
            moved += 1

    for obj, dest_path in pending_moves:
        result = _move_asset(obj, dest_path)
        if _to_asset_path(result.get_path_name()) == dest_path:
            moved += 1

    unreal.log(f"{_LOG_PREFIX} Stage 4: {moved} of {len(moves) + len(pending_moves)} assets moved.")


# ---------------------------------------------------------------------------
# Stages 5-7 - main ABP, Character BP subclass, Control Rig
# ---------------------------------------------------------------------------

def _stage_create_main_abp(output_root, target_skeleton, als_name_replace, settings_asset):
    """Creates AB_{alsNameReplace} at the output root, parented to MAIN_ANIM_INST_CLASS."""
    parent_class = unreal.load_class(None, f"/Script/{MAIN_ANIM_INST_CLASS}")
    if not parent_class:
        unreal.log_error(f"{_LOG_PREFIX} Could not load native class '{MAIN_ANIM_INST_CLASS}'")
        return None

    asset_name = f"AB_{als_name_replace}"
    build_path = f"{output_root}/{asset_name}"
    if _asset_lib.does_asset_exist(build_path):
        _asset_lib.delete_asset(build_path)

    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    factory.set_editor_property("target_skeleton", target_skeleton)
    factory.set_editor_property("template", False)
    main_abp = _asset_tools.create_asset(asset_name=asset_name, package_path=output_root, asset_class=None, factory=factory)
    if not main_abp:
        unreal.log_error(f"{_LOG_PREFIX} Failed to create main ABP '{build_path}'")
        return None

    cdo = unreal.get_default_object(main_abp.generated_class())
    if settings_asset:
        _set_property(cdo, SETTINGS_VAR, settings_asset, f"main ABP '{asset_name}'")
    else:
        unreal.log_warning(f"{_LOG_PREFIX} No retargeted '{AIS_DEFAULT_PATH}' found for the main ABP's Settings")

    interface_bp = unreal.load_asset(ALI_OVERLAY_INTERFACE_PATH)
    interface_class = interface_bp.generated_class() if interface_bp else None
    if not interface_class:
        unreal.log_warning(f"{_LOG_PREFIX} Interface not found: '{ALI_OVERLAY_INTERFACE_PATH}'")
    elif not _ALS_LIB.implement_interface(main_abp, interface_class):
        unreal.log_warning(f"{_LOG_PREFIX} Could not implement '{ALI_OVERLAY_INTERFACE_PATH}' on '{asset_name}'")

    unreal.BlueprintEditorLibrary.compile_blueprint(main_abp)
    unreal.log(f"{_LOG_PREFIX} Stage 5: created main ABP '{build_path}'.")
    return main_abp


def _resolve_overlay_class(original_class, abt_instance_by_template):
    """Maps an original AB_Als_* overlay class onto its newly instanced ABT's class."""
    if original_class is None:
        return None
    name = original_class.get_name()
    if name.endswith("_C"):
        name = name[:-2]
    if not name.startswith(ABT_INSTANCE_NAME_PREFIX):
        return None

    template_name = ABT_TEMPLATE_NAME_PREFIX + name[len(ABT_INSTANCE_NAME_PREFIX):]
    instance_abt = abt_instance_by_template.get(template_name)
    if not instance_abt:
        unreal.log_warning(f"{_LOG_PREFIX} No new instance found for overlay class '{name}' (template '{template_name}')")
        return None
    return instance_abt.generated_class()


def _configure_character_mesh(cdo, target_skel_mesh, main_abp, label):
    """Points the inherited ACharacter Mesh component at the new skeletal mesh and animation blueprint."""
    mesh_component = _get_property(cdo, CHARACTER_MESH_VAR, label)
    if mesh_component is None:
        unreal.log_error(f"{_LOG_PREFIX} '{CHARACTER_MESH_VAR}' on {label} is None - cannot configure the mesh")
        return

    mesh_label = f"{label} Mesh component"
    if target_skel_mesh:
        _try_set_property(mesh_component, "SkeletalMeshAsset", target_skel_mesh, mesh_label)
    else:
        unreal.log_warning(f"{_LOG_PREFIX} No target skeletal mesh to assign on {mesh_label}")

    if main_abp:
        _try_set_property(mesh_component, "AnimationMode", unreal.AnimationMode.ANIMATION_BLUEPRINT, mesh_label)
        _try_set_property(mesh_component, "AnimClass", main_abp.generated_class(), mesh_label)
    else:
        unreal.log_warning(f"{_LOG_PREFIX} No main ABP was created - {mesh_label} keeps its inherited Anim Class")


def _stage_create_character_subclass(output_root, als_name_replace, abt_instance_by_template, target_skel_mesh, main_abp):
    """Subclasses B_Als_Character, repointing Settings, mantle settings, overlays and the Mesh component."""
    template_bp = unreal.load_asset(ALS_CHARACTER_BP_PATH)
    if not template_bp:
        unreal.log_error(f"{_LOG_PREFIX} Character template not found: '{ALS_CHARACTER_BP_PATH}'")
        return None

    asset_name = _case_insensitive_replace(template_bp.get_name(), _ALS_SEARCH_LONG, f"_{als_name_replace}_")
    build_path = f"{output_root}/{asset_name}"
    if _asset_lib.does_asset_exist(build_path):
        _asset_lib.delete_asset(build_path)

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", template_bp.generated_class())
    subclass_bp = _asset_tools.create_asset(asset_name=asset_name, package_path=output_root, asset_class=None, factory=factory)
    if not subclass_bp:
        unreal.log_error(f"{_LOG_PREFIX} Failed to create character subclass '{build_path}'")
        return None

    cdo = unreal.get_default_object(subclass_bp.generated_class())
    label = f"character subclass '{asset_name}'"

    cs_source = unreal.load_asset(CS_DEFAULT_PATH)
    cs_replacement = _DATA_ASSET_REPLACEMENTS.get(cs_source) if cs_source else None
    if not cs_replacement:
        raise RuntimeError(
            f"{_LOG_PREFIX} No retargeted counterpart for '{CS_DEFAULT_PATH}' - it must be listed in "
            f"DATA_ASSETS_TO_RETARGET and duplicated in stage 3 before '{SETTINGS_VAR}' can be set on {label}"
        )
    _set_property(cdo, SETTINGS_VAR, cs_replacement, label)

    for var_name in MANTLE_SETTINGS_VARS:
        replacement = _DATA_ASSET_REPLACEMENTS.get(_get_property(cdo, var_name, label))
        if replacement:
            _set_property(cdo, var_name, replacement, label)
        else:
            unreal.log_warning(f"{_LOG_PREFIX} '{var_name}' on {label} has no retargeted counterpart - left unchanged")

    # Collect the overlay updates first; mutating an unreal.Map while iterating it is unsafe.
    overlay_map = _get_property(cdo, OVERLAY_MAP_VAR, label)
    updates = {}
    for tag, overlay_class in list(overlay_map.items()):
        resolved = _resolve_overlay_class(overlay_class, abt_instance_by_template)
        if resolved:
            updates[tag] = resolved
    for tag, resolved in updates.items():
        overlay_map[tag] = resolved
    _set_property(cdo, OVERLAY_MAP_VAR, overlay_map, label)
    unreal.log(f"{_LOG_PREFIX} Repointed {len(updates)} of {len(overlay_map)} entries in '{OVERLAY_MAP_VAR}'.")

    resolved_default = _resolve_overlay_class(_get_property(cdo, DEFAULT_OVERLAY_VAR, label), abt_instance_by_template)
    if resolved_default:
        _set_property(cdo, DEFAULT_OVERLAY_VAR, resolved_default, label)
    else:
        unreal.log_warning(f"{_LOG_PREFIX} '{DEFAULT_OVERLAY_VAR}' on {label} could not be repointed - left unchanged")

    _configure_character_mesh(cdo, target_skel_mesh, main_abp, label)

    unreal.BlueprintEditorLibrary.compile_blueprint(subclass_bp)
    _asset_lib.save_loaded_asset(subclass_bp, only_if_is_dirty=False)

    unreal.log(f"{_LOG_PREFIX} Stage 6: created character subclass '{build_path}'.")
    return subclass_bp


def _stage_copy_control_rig(output_root, als_name_replace, target_skel_mesh):
    """Copies CR_Als to the output root, setting its preview mesh and refreshing its hierarchy."""
    source_cr = unreal.load_asset(ALS_CONTROL_RIG_PATH)
    if not source_cr:
        unreal.log_error(f"{_LOG_PREFIX} Control Rig not found: '{ALS_CONTROL_RIG_PATH}'")
        return None

    asset_name = _case_insensitive_replace(source_cr.get_name(), _ALS_SEARCH_SHORT, f"_{als_name_replace}")
    build_path = f"{output_root}/{asset_name}"
    if _asset_lib.does_asset_exist(build_path):
        _asset_lib.delete_asset(build_path)

    new_cr = _asset_lib.duplicate_asset(_to_asset_path(ALS_CONTROL_RIG_PATH), build_path)
    if not new_cr:
        unreal.log_error(f"{_LOG_PREFIX} Failed to duplicate Control Rig -> '{build_path}'")
        return None

    if target_skel_mesh:
        setter = getattr(new_cr, "set_preview_mesh", None)
        if callable(setter):
            setter(target_skel_mesh)
        else:
            _try_set_property(new_cr, "PreviewSkeletalMesh", target_skel_mesh, f"Control Rig '{asset_name}'")
    else:
        unreal.log_warning(f"{_LOG_PREFIX} No target skeletal mesh - preview mesh left as-is on '{asset_name}'")

    # Best-effort: this Python surface is not stable across engine builds, so skip cleanly if unavailable.
    try:
        hierarchy = new_cr.get_editor_property("hierarchy")
        controller = hierarchy.get_controller() if hierarchy else None
        skeleton = target_skel_mesh.get_editor_property("skeleton") if target_skel_mesh else None
        if controller and skeleton and hasattr(controller, "import_bones"):
            controller.import_bones(skeleton, "", True, False, True, True)
        else:
            unreal.log_warning(f"{_LOG_PREFIX} No Python access to refresh the rig hierarchy - do this manually for '{asset_name}'")
    except Exception as exc:
        unreal.log_warning(f"{_LOG_PREFIX} Hierarchy refresh failed for '{asset_name}': {exc}")

    unreal.log(f"{_LOG_PREFIX} Stage 7: copied Control Rig '{build_path}'.")
    return new_cr


# ---------------------------------------------------------------------------
# Stage 8 - fix up redirectors
# ---------------------------------------------------------------------------

def _find_redirectors_under(roots):
    """Returns the package path of every ObjectRedirector under roots, excluding REDIRECTORS_TO_KEEP."""
    paths = []
    for root in roots:
        root = _norm(root)
        if not _asset_lib.does_directory_exist(root):
            continue
        for asset_data in _asset_registry.get_assets_by_path(root, recursive=True):
            if _asset_class_name(asset_data) != _REDIRECTOR_CLASS_NAME:
                continue
            package_name = str(asset_data.package_name)
            if _name_of(package_name) in REDIRECTORS_TO_KEEP or package_name in paths:
                continue
            paths.append(package_name)
    return paths


def _load_redirector(package_path):
    """Loads the ObjectRedirector at package_path without resolving through it."""
    return unreal.load_asset(f"{package_path}.{_name_of(package_path)}", follow_redirectors=False)


def _referencers_of(package_path):
    """Returns the packages still referencing package_path."""
    options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True,
        include_hard_package_references=True,
    )
    referencers = _asset_registry.get_referencers(package_path, options)
    return [str(name) for name in referencers] if referencers else []


def _delete_redirectors(paths):
    """Deletes redirectors nothing references any more, reporting the rest with their referencers."""
    deleted = 0
    for path in paths:
        referencers = _referencers_of(path)
        if referencers:
            unreal.log_warning(f"{_LOG_PREFIX} '{path}' still referenced by: {', '.join(referencers[:5])}")
        elif _asset_lib.delete_asset(path):
            deleted += 1
        else:
            unreal.log_warning(f"{_LOG_PREFIX} Failed to delete unreferenced redirector '{path}'")
    return deleted


def _stage_fixup_redirectors(roots):
    """Fixes up every redirector under roots except REDIRECTORS_TO_KEEP, then deletes them."""
    scan_roots = [_norm(root) for root in roots if root]

    def _rescan():
        _asset_registry.scan_paths_synchronous(scan_roots, force_rescan=True)
        _asset_registry.wait_for_completion()

    _rescan()
    paths = _find_redirectors_under(roots)
    if not paths:
        unreal.log(f"{_LOG_PREFIX} Stage 8: no redirectors to fix up.")
        return

    unreal.log(f"{_LOG_PREFIX} Stage 8: fixing up {len(paths)} redirectors, leaving {', '.join(REDIRECTORS_TO_KEEP)} in place...")
    redirectors = [_load_redirector(path) for path in paths]
    _ALS_LIB.fixup_referencers(redirectors)
    _save_dirty_packages()

    # The Python wrappers keep the redirectors alive, so FixupReferencers cannot delete them itself -
    # drop our references and collect before deleting them by path.
    del redirectors
    unreal.SystemLibrary.collect_garbage()
    _rescan()

    remaining = _find_redirectors_under(roots)
    if remaining:
        deleted = _delete_redirectors(remaining)
        unreal.log(f"{_LOG_PREFIX} Stage 8: deleted {deleted} of {len(remaining)} redirectors left after the fixup.")
        _rescan()
        remaining = _find_redirectors_under(roots)

    if not remaining:
        unreal.log(f"{_LOG_PREFIX} Stage 8: all redirectors cleared.")
        return

    unreal.log_warning(f"{_LOG_PREFIX} Stage 8: {len(remaining)} redirectors remain:")
    for path in remaining:
        unreal.log_warning(f"{_LOG_PREFIX}   {path}")


# ---------------------------------------------------------------------------
# Orchestrator
# ---------------------------------------------------------------------------

def retarget(outputDir, targetSkel, targetSkelIkRtg, targetSkelMesh, alsNameReplace):
    """Runs the full retarget pipeline, committing the session between stages."""
    _REPLACEMENT_OBJECTS.clear()

    als_name_replace = _normalize_als_name_replace(alsNameReplace)
    als_long_replace = f"_{als_name_replace}_"

    abt_dir = _norm(ABT_DIR)
    anims_root = _norm(ANIMS_ROOT)
    data_root = _norm(DATA_ASSETS_ROOT)
    output_root = _norm(outputDir)

    abt_instance_dir = f"{output_root}/{_name_of(abt_dir)}"
    anims_rtg_root = f"{output_root}/{_name_of(anims_root)}"
    data_rtg_root = f"{output_root}/Data"

    commit_paths = [anims_root, data_root, abt_dir, output_root]

    redirector_roots = [_norm(ALS_ROOT)]
    if not output_root.startswith(_norm(ALS_ROOT) + "/"):
        redirector_roots.append(output_root)

    target_skeleton = unreal.load_asset(targetSkel)
    ik_retargeter = unreal.load_asset(targetSkelIkRtg)
    target_skel_mesh = unreal.load_asset(targetSkelMesh)

    expected_names = _stage_retarget_animations(anims_root, ik_retargeter, _ALS_SEARCH_LONG, als_long_replace)
    _commit("stage 1 (retarget)", commit_paths)

    retarget_map = _stage_deduplicate_animations(anims_root, expected_names)
    _commit("stage 2 (deduplicate)", commit_paths)

    pending_moves = _stage_process_data_assets(data_root, data_rtg_root, als_name_replace)
    abt_moves, abt_instance_by_template = _stage_process_abts(abt_dir, abt_instance_dir, target_skeleton, retarget_map, als_name_replace)
    pending_moves.extend(abt_moves)
    _commit("stage 3 (ABTs and Data Assets)", commit_paths)

    abt_dest_paths = [dest_path for _, dest_path in abt_moves]
    _stage_move_to_output(retarget_map, anims_root, anims_rtg_root, pending_moves)

    instanced_abts = []
    for dest_path in abt_dest_paths:
        final_abt = unreal.load_asset(dest_path)
        if final_abt:
            unreal.BlueprintEditorLibrary.compile_blueprint(final_abt)
            instanced_abts.append(final_abt)

    _commit("stage 4 (move to output)", commit_paths)

    settings_asset = _DATA_ASSET_REPLACEMENTS.get(unreal.load_asset(AIS_DEFAULT_PATH))
    main_abp = _stage_create_main_abp(output_root, target_skeleton, als_name_replace, settings_asset)
    character_subclass = _stage_create_character_subclass(
        output_root, als_name_replace, abt_instance_by_template, target_skel_mesh, main_abp
    )
    control_rig = _stage_copy_control_rig(output_root, als_name_replace, target_skel_mesh)
    _commit("stages 5-7 (main ABP, character subclass, Control Rig)", commit_paths)

    _stage_fixup_redirectors(redirector_roots)
    _commit("stage 8 (redirector fixup)", commit_paths)

    unreal.log(
        f"{_LOG_PREFIX} Finished! {len(instanced_abts)} ABT instances, {len(DATA_ASSETS_TO_RETARGET)} Data Assets, "
        f"{len(retarget_map)} retargeted animations, main ABP: {bool(main_abp)}, "
        f"character subclass: {bool(character_subclass)}, Control Rig: {bool(control_rig)}."
    )
    return instanced_abts, list(retarget_map.values()), main_abp, character_subclass, control_rig
