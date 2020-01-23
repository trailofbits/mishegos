meta:
  id: cohort
  endian: le

seq:
  - id: total_cohorts
    type: entry
    repeat: eos
    
types:
  entry: 
    seq: 
      - id: nworkers
        type: u4
      - id: input_length
        type: u8
      - id: input
        size: input_length
        type: str
        encoding: ascii
      - id: type
        type: cohort_type
        repeat: expr
        repeat-expr: nworkers
  cohort_type:
    seq:
      - id: status
        type: u4
        enum: decode_status
      - id: ndecoded
        type: u2
      - id: workerno
        type: u4
      - id: worker_so_length
        type: u8
      - id: worker_so
        size: worker_so_length
        type: str
        encoding: ascii 
      - id: len
        type: u2
      - id: result
        size: len
        type: str
        encoding: ascii 
        
enums:
  decode_status:
    0: s_none
    1: s_success
    2: s_failure
    3: s_crash
    4: s_hang
    5: s_partial
    6: s_wouldblock
    7: s_unknown
      
      

  
  
