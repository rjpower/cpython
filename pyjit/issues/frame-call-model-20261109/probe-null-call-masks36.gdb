# Coordinator only. Use the matching debug36 ELF and original two test methods
# as --args. This command file only reads inferior state; it calls no function.
set pagination off
set confirm off
set print pretty off
set print elements 32
set breakpoint pending on
set $null36_hits = 0
break register_resume_plan if ((current == 4 && target == 21 && analysis->nl == 1) || (current == 33 && target == 4 && analysis->nl == 4))
commands
  silent
  if $null36_hits < 8
    set $null36_hits = $null36_hits + 1
    printf "NULL36_PLAN hit=%d current=%d target=%d frame=%p\n", $null36_hits, current, target, frame
    printf "geometry n=%d nl=%d nt=%d stride=%lu phase=%d phase_pc=%d context=%d target_context=%d saves=%d safe_target=%d\n", analysis->n, analysis->nl, analysis->nt, (unsigned long)analysis->stride, phase.kind, phase.pc, phase.context, analysis->context[target], analysis->nsave_columns, analysis->safe[target]
    printf "physical_pc=%ld return_pc=%ld\n", (long)(frame->instr_ptr-frame->ir_words), (long)(frame->return_pc-frame->ir_words)
    set $null36_j = 0
    while $null36_j < analysis->nt && $null36_j < 32
      set $null36_home = analysis->nl + $null36_j
      set $null36_mask = (unsigned long long)1 << ($null36_j % 64)
      set $null36_target_index = target * analysis->stride + $null36_j / 64
      set $null36_current_index = current * analysis->stride + $null36_j / 64
      printf "home=%d bits=0x%llx ALL=%d", $null36_home, (unsigned long long)frame->base[$null36_home].bits, (analysis->required[$null36_target_index] & $null36_mask) != 0
      if analysis->value_required != 0 && analysis->null_required != 0 && analysis->null_states != 0
        printf " VALUE=%d NULL_ONLY=%d current_must_null=%d target_must_null=%d\n", (analysis->value_required[$null36_target_index] & $null36_mask) != 0, (analysis->null_required[$null36_target_index] & $null36_mask) != 0, (analysis->null_states[$null36_current_index] & $null36_mask) != 0, (analysis->null_states[$null36_target_index] & $null36_mask) != 0
      else
        printf " pathwise_lanes=unavailable\n"
      end
      set $null36_j = $null36_j + 1
    end
    set $null36_j = 0
    while $null36_j < analysis->nsave_columns && $null36_j < 16
      printf "save_column=%d current_top=%d target_top=%d\n", $null36_j, phase.save_tops[$null36_j], analysis->save_tops[target*analysis->nsave_columns+$null36_j]
      set $null36_j = $null36_j + 1
    end
    printf "END_NULL36_PLAN\n"
  end
  continue
end
run
