define_property(GLOBAL PROPERTY ATTRFL_TARGETS
        BRIEF_DOCS "all CMake targets that have called attrfl_gen"
)

define_property(GLOBAL PROPERTY ATTRFL_ATTACH_TARGETS
        BRIEF_DOCS "the attachTarget corresponding to each attrfl target"
)


function(_attrfl_collect_deps target attrfl_targets_list out_var)
    set(_result "")
    get_target_property(_links ${target} LINK_LIBRARIES)
    if (NOT _links)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif ()

    foreach (_lib IN LISTS _links)
        if (NOT TARGET ${_lib})
            continue()
        endif ()
        list(FIND attrfl_targets_list "${_lib}" _idx)
        if (_idx EQUAL -1)
            continue()
        endif ()
        # 加入结果
        list(APPEND _result "${_lib}")
        # 递归收集该依赖的依赖
        _attrfl_collect_deps(${_lib} "${attrfl_targets_list}" _sub_result)
        if (_sub_result)
            list(APPEND _result ${_sub_result})
        endif ()
    endforeach ()

    list(REMOVE_DUPLICATES _result)
    set(${out_var} "${_result}" PARENT_SCOPE)
endfunction()

function(_attrfl_deferred_gen target attachTarget)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "attrfl_gen : target '${target}' not found")
    endif ()

    set(collect_dir "${CMAKE_BINARY_DIR}/generate/${target}")
    file(MAKE_DIRECTORY "${collect_dir}")

    set(out_json "${collect_dir}/classes.json")
    set(generate_source "${collect_dir}/register_reflect_${target}_classes.cpp")
    set(flags_file "${collect_dir}/flags.txt")
    set(include_dirs_file "${collect_dir}/include_dirs.txt")
    set(stamp_file "${collect_dir}/.stamp")

    get_target_property(_target_type ${target} TYPE)
    if (_target_type STREQUAL "INTERFACE_LIBRARY")
        set(_inc_prop "INTERFACE_INCLUDE_DIRECTORIES")
        set(_def_prop "INTERFACE_COMPILE_DEFINITIONS")
    else ()
        set(_inc_prop "INCLUDE_DIRECTORIES")
        set(_def_prop "COMPILE_DEFINITIONS")
    endif ()

    file(GENERATE
            OUTPUT "${include_dirs_file}"
            CONTENT "$<JOIN:$<TARGET_PROPERTY:${target},${_inc_prop}>,\n>"
    )

    file(GENERATE
            OUTPUT "${flags_file}"
            CONTENT "-std=c++${CMAKE_CXX_STANDARD}\n$<JOIN:$<LIST:TRANSFORM,$<TARGET_PROPERTY:${target},${_def_prop}>,PREPEND,-D>,\n>"
    )

    set(_root_dirs "")
    get_target_property(_inc_props ${target} INCLUDE_DIRECTORIES)
    get_target_property(_iface_inc_props ${target} INTERFACE_INCLUDE_DIRECTORIES)

    set(_all_inc "")
    if (_inc_props)
        list(APPEND _all_inc ${_inc_props})
    endif ()
    if (_iface_inc_props)
        list(APPEND _all_inc ${_iface_inc_props})
    endif ()

    if (_all_inc)
        foreach (_inc IN LISTS _all_inc)
            if ("${_inc}" MATCHES "^\\$<")
                continue()
            endif ()
            if (_inc STREQUAL "")
                continue()
            endif ()
            if (NOT IS_ABSOLUTE "${_inc}")
                get_filename_component(_inc_abs "${CMAKE_CURRENT_LIST_DIR}/${_inc}" REALPATH)
                if (_inc_abs)
                    list(APPEND _root_dirs "${_inc_abs}")
                else ()
                    list(APPEND _root_dirs "${CMAKE_SOURCE_DIR}/${_inc}")
                endif ()
            else ()
                list(APPEND _root_dirs "${_inc}")
            endif ()
        endforeach ()
    endif ()

    if (_root_dirs STREQUAL "")
        list(APPEND _root_dirs "${CMAKE_SOURCE_DIR}")
    endif ()
    list(REMOVE_DUPLICATES _root_dirs)
    string(JOIN "|" _roots_arg ${_root_dirs})

    get_property(_attrfl_targets GLOBAL PROPERTY ATTRFL_TARGETS)

    _attrfl_collect_deps(${target} "${_attrfl_targets}" _dep_targets)

    set(_deps_arg "")
    foreach (_dep IN LISTS _dep_targets)
        set(_dep_json "${CMAKE_BINARY_DIR}/generate/${_dep}/classes.json")
        if (_deps_arg STREQUAL "")
            set(_deps_arg "${_dep_json}")
        else ()
            string(APPEND _deps_arg "|${_dep_json}")
        endif ()
    endforeach ()

    if (EXISTS "${ATTRFL_EXE}")
        set(_gen_cmd
                ${ATTRFL_EXE} --generate
                --include_path "${include_dirs_file}"
                --flags "${flags_file}"
                --out "${out_json}"
                --out_cpp "${generate_source}"
                --target "${target}"
                --roots "${_roots_arg}"
                --stamp "${stamp_file}"
        )
        if (NOT _deps_arg STREQUAL "")
            list(APPEND _gen_cmd --deps "${_deps_arg}")
        endif ()
        if (ATTRFL_MULTITHREAD)
            list(APPEND _gen_cmd --MT)
        endif ()
        if (ATTRFL_NOSTDINC)
            list(APPEND _gen_cmd --NOSTDINC)
        endif ()

        add_custom_command(
                OUTPUT "${generate_source}" "${stamp_file}"
                COMMAND ${_gen_cmd}
                COMMENT "attrfl: generate for ${target}"
                WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                DEPENDS "${include_dirs_file}" "${flags_file}"
                VERBATIM
        )

        add_custom_target(attrfl_${target}
                DEPENDS "${generate_source}" "${stamp_file}"
        )

        foreach (_dep IN LISTS _dep_targets)
            add_dependencies(attrfl_${target} attrfl_${_dep})
        endforeach ()

        set_source_files_properties(${generate_source} PROPERTIES GENERATED TRUE)
        target_sources(${attachTarget} PRIVATE ${generate_source})
        add_dependencies(${attachTarget} attrfl_${target})
    else ()
        message(SEND_ERROR "ATTRFL_EXE (${ATTRFL_EXE}) not found")
    endif ()
endfunction()


function(_attrfl_create_force_all_target targets_list)
    if (NOT EXISTS "${ATTRFL_EXE}")
        return()
    endif ()

    list(LENGTH targets_list _count)
    if (_count EQUAL 0)
        return()
    endif ()
    math(EXPR _last "${_count} - 1")

    set(_force_commands "")
    foreach (_i RANGE ${_last})
        list(GET targets_list ${_i} _target)

        set(_collect_dir "${CMAKE_BINARY_DIR}/generate/${_target}")

        set(_cmd
                COMMAND ${ATTRFL_EXE} --generate --force
                --include_path "${_collect_dir}/include_dirs.txt"
                --flags "${_collect_dir}/flags.txt"
                --out "${_collect_dir}/classes.json"
                --out_cpp "${_collect_dir}/register_reflect_${_target}_classes.cpp"
                --target "${_target}"
                --stamp "${_collect_dir}/.stamp"
        )

        set(_root_dirs "")
        get_target_property(_inc_props ${_target} INCLUDE_DIRECTORIES)
        get_target_property(_iface_inc_props ${_target} INTERFACE_INCLUDE_DIRECTORIES)

        set(_all_inc "")
        if (_inc_props)
            list(APPEND _all_inc ${_inc_props})
        endif ()
        if (_iface_inc_props)
            list(APPEND _all_inc ${_iface_inc_props})
        endif ()

        if (_all_inc)
            foreach (_inc IN LISTS _all_inc)
                if ("${_inc}" MATCHES "^\\$<" OR _inc STREQUAL "")
                    continue()
                endif ()
                if (NOT IS_ABSOLUTE "${_inc}")
                    get_filename_component(_inc_abs "${CMAKE_CURRENT_LIST_DIR}/${_inc}" REALPATH)
                    list(APPEND _root_dirs "${_inc_abs}")
                else ()
                    list(APPEND _root_dirs "${_inc}")
                endif ()
            endforeach ()
        endif ()
        if (_root_dirs STREQUAL "")
            list(APPEND _root_dirs "${CMAKE_SOURCE_DIR}")
        endif ()
        list(REMOVE_DUPLICATES _root_dirs)
        string(JOIN "|" _roots_arg ${_root_dirs})
        list(APPEND _cmd --roots "${_roots_arg}")

        _attrfl_collect_deps(${_target} "${targets_list}" _dep_targets)
        set(_deps_arg "")
        foreach (_dep IN LISTS _dep_targets)
            if (_deps_arg STREQUAL "")
                set(_deps_arg "${CMAKE_BINARY_DIR}/generate/${_dep}/classes.json")
            else ()
                string(APPEND _deps_arg "|${CMAKE_BINARY_DIR}/generate/${_dep}/classes.json")
            endif ()
        endforeach ()
        if (NOT _deps_arg STREQUAL "")
            list(APPEND _cmd --deps "${_deps_arg}")
        endif ()
        if (ATTRFL_MULTITHREAD)
            list(APPEND _cmd --MT)
        endif ()
        if (ATTRFL_NOSTDINC)
            list(APPEND _cmd --NOSTDINC)
        endif ()

        list(APPEND _force_commands ${_cmd})
    endforeach ()
    add_custom_target(attrfl_force_all
            ${_force_commands}
            COMMENT "attrfl: force rebuild ALL targets"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            VERBATIM
    )
endfunction()


function(attrfl_gen target attachTarget)
    set_property(GLOBAL APPEND PROPERTY ATTRFL_TARGETS "${target}")
    set_property(GLOBAL APPEND PROPERTY ATTRFL_ATTACH_TARGETS "${attachTarget}")
endfunction()

function(attrfl_finalize)
    get_property(_targets GLOBAL PROPERTY ATTRFL_TARGETS)
    get_property(_attach_targets GLOBAL PROPERTY ATTRFL_ATTACH_TARGETS)

    list(LENGTH _targets _count)
    math(EXPR _last "${_count} - 1")

    foreach (_i RANGE ${_last})
        list(GET _targets ${_i} _target)
        list(GET _attach_targets ${_i} _attach)
        _attrfl_deferred_gen("${_target}" "${_attach}")
    endforeach ()

    _attrfl_create_force_all_target("${_targets}")
endfunction()

function(attrfl_base_register target)
    set(base_type_generate_source "${CMAKE_BINARY_DIR}/generate/register_reflect_base_type.cpp")
    add_custom_command(
            OUTPUT ${base_type_generate_source}
            COMMAND ${ATTRFL_EXE} --baseType_register --out "${base_type_generate_source}"
            COMMENT "Command: BaseTypeRegister for ${target}"
            VERBATIM
    )
    set_source_files_properties(${base_type_generate_source} PROPERTIES GENERATED TRUE)
    target_sources("${target}" PRIVATE ${base_type_generate_source})
endfunction()
