cmd_/workspace/src/02_modules/exercice08/modules.order := {   echo /workspace/src/02_modules/exercice08/mymodule.ko; :; } | awk '!x[$$0]++' - > /workspace/src/02_modules/exercice08/modules.order
