cmake_minimum_required(VERSION 3.28)

project(ArcaneSpire VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "Audio will be enabled after the graphics probe" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "Networking is outside the course scope" FORCE)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SFML)

add_library(arcane_common INTERFACE)
target_include_directories(arcane_common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Project1/src
)

function(arcane_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(arcane_core STATIC
    Project1/src/game/ai/EnemyController.cpp
    Project1/src/app/run/TowerSession.cpp
    Project1/src/game/combat/AttackState.cpp
    Project1/src/game/combat/CombatSession.cpp
    Project1/src/game/combat/CombatSessionBossRules.cpp
    Project1/src/game/combat/CombatSessionEnemySkills.cpp
    Project1/src/game/combat/CombatSessionSpellEffects.cpp
    Project1/src/game/combat/DamageResolver.cpp
    Project1/src/game/combat/Health.cpp
    Project1/src/game/player/PlayerController.cpp
    Project1/src/game/progression/ProgressionSystem.cpp
    Project1/src/game/run/DeterministicRng.cpp
    Project1/src/game/relics/RelicSystem.cpp
    Project1/src/game/spells/SpellSystem.cpp
    Project1/src/game/floors/ArenaLayout.cpp
    Project1/src/game/floors/FloorController.cpp
    Project1/src/game/floors/FloorScheduler.cpp
    Project1/src/game/rewards/RewardSystem.cpp
    Project1/src/game/economy/MerchantSystem.cpp
    Project1/src/game/events/EventSystem.cpp
    Project1/src/app/run/RunController.cpp
)

target_link_libraries(arcane_core
    PUBLIC
        arcane_common
)

add_library(arcane_viewmodel STATIC
    Project1/src/presentation/viewmodel/UiViewModels.cpp
    Project1/src/presentation/viewmodel/LoadoutViewModel.cpp
    Project1/src/presentation/viewmodel/ApplicationViewModel.cpp
    Project1/src/presentation/viewmodel/CombatFeedbackViewModel.cpp
    Project1/src/presentation/viewmodel/SpellAcquisitionViewModel.cpp
)

target_link_libraries(arcane_viewmodel
    PUBLIC
        arcane_common
        arcane_core
)

add_library(arcane_sfml_view STATIC
    Project1/src/presentation/EnemyAnimator.cpp
    Project1/src/presentation/LootBookAnimator.cpp
    Project1/src/presentation/PlayerAnimator.cpp
    Project1/src/presentation/ShadeChargeAnimator.cpp
    Project1/src/presentation/SpellCardArt.cpp
    Project1/src/presentation/SpellEffectAnimator.cpp
    Project1/src/presentation/views/ArenaView.cpp
    Project1/src/presentation/views/UiPrimitives.cpp
    Project1/src/presentation/views/ScreenViews.cpp
    Project1/src/presentation/views/StaircaseView.cpp
    Project1/src/presentation/views/CombatView.cpp
    Project1/src/presentation/views/SpellAcquisitionView.cpp
)

target_link_libraries(arcane_sfml_view
    PUBLIC
        arcane_common
        SFML::Graphics
        SFML::Window
        SFML::System
)

add_executable(Project1
    Project1/src/main.cpp
    Project1/src/platform/SfmlInputMapper.cpp
    Project1/src/presentation/SfmlApplication.cpp
)

target_link_libraries(Project1
    PRIVATE
        arcane_common
        arcane_viewmodel
        arcane_sfml_view
        SFML::Graphics
        SFML::Window
        SFML::System
)

add_custom_command(TARGET Project1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Project1>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Project1/assets/player/processed
        $<TARGET_FILE_DIR:Project1>/assets/player
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Project1/assets/spells/processed
        $<TARGET_FILE_DIR:Project1>/assets/spells
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Project1/assets/spell_cards
        $<TARGET_FILE_DIR:Project1>/assets/spell_cards
    COMMENT "Copying runtime assets"
)

arcane_enable_warnings(arcane_core)
arcane_enable_warnings(arcane_viewmodel)
arcane_enable_warnings(arcane_sfml_view)
arcane_enable_warnings(Project1)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Project1)

include(CTest)

if(BUILD_TESTING)
    add_executable(arcane_core_tests
        tests/player_controller_tests.cpp
    )
    target_link_libraries(arcane_core_tests PRIVATE arcane_core)

    add_executable(combat_session_tests
        tests/combat_session_tests.cpp
    )
    target_link_libraries(combat_session_tests PRIVATE arcane_core)

    add_executable(run_flow_tests
        tests/run_flow_tests.cpp
    )
    target_link_libraries(run_flow_tests PRIVATE arcane_viewmodel)

    add_executable(tower_session_tests
        tests/tower_session_tests.cpp
    )
    target_link_libraries(tower_session_tests PRIVATE arcane_viewmodel)

    add_executable(arena_layout_tests
        tests/arena_layout_tests.cpp
    )
    target_link_libraries(arena_layout_tests PRIVATE arcane_core)

    add_executable(sfml_input_mapper_tests
        tests/sfml_input_mapper_tests.cpp
        Project1/src/platform/SfmlInputMapper.cpp
    )
    target_include_directories(sfml_input_mapper_tests PRIVATE Project1/src)
    target_link_libraries(sfml_input_mapper_tests PRIVATE arcane_common SFML::Window SFML::System)

    add_executable(binding_contract_tests
        tests/binding_contract_tests.cpp
    )
    target_link_libraries(binding_contract_tests PRIVATE arcane_viewmodel)

    arcane_enable_warnings(arcane_core_tests)
    arcane_enable_warnings(combat_session_tests)
    arcane_enable_warnings(run_flow_tests)
    arcane_enable_warnings(tower_session_tests)
    arcane_enable_warnings(arena_layout_tests)
    arcane_enable_warnings(sfml_input_mapper_tests)
    arcane_enable_warnings(binding_contract_tests)

    add_test(NAME player_controller COMMAND arcane_core_tests)
    add_test(NAME combat_session COMMAND combat_session_tests)
    add_test(NAME run_flow COMMAND run_flow_tests)
    add_test(NAME tower_session COMMAND tower_session_tests)
    add_test(NAME arena_layout COMMAND arena_layout_tests)
    add_test(NAME sfml_input_mapper COMMAND sfml_input_mapper_tests)
    add_test(NAME binding_contract COMMAND binding_contract_tests)
    add_test(NAME view_dependency_boundary
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyViewDependencies.cmake)
endif()
