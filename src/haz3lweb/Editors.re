open Sexplib.Std;
open Haz3lcore;

[@deriving (show({with_path: false}), sexp, yojson)]
type t =
  | DebugLoad
  | Scratch(int, list(ScratchSlide.state))
  | School(int, list(SchoolExercise.spec), SchoolExercise.state);

[@deriving (show({with_path: false}), sexp, yojson)]
type scratch = (int, list(ScratchSlide.state));

[@deriving (show({with_path: false}), sexp, yojson)]
type school = (int, list(SchoolExercise.spec), SchoolExercise.state);

[@deriving (show({with_path: false}), sexp, yojson)]
type mode =
  | DebugLoad
  | Scratch
  | School;

let rotate_mode = (editors: t) =>
  switch (editors) {
  | DebugLoad => DebugLoad
  | Scratch(_) => School
  | School(_) => Scratch
  };

let get_editor_and_id = (editors: t): (Id.t, Editor.t) =>
  switch (editors) {
  | DebugLoad => failwith("no editors in debug load mode")
  | Scratch(n, slides) =>
    assert(n < List.length(slides));
    let slide = List.nth(slides, n);
    let id = ScratchSlide.id_of_state(slide);
    let ed = ScratchSlide.editor_of_state(slide);
    (id, ed);
  | School(_, _, exercise) =>
    let id = SchoolExercise.id_of_state(exercise);
    let ed = SchoolExercise.editor_of_state(exercise);
    (id, ed);
  };

let get_editor = (editors: t): Editor.t => snd(get_editor_and_id(editors));

let put_editor_and_id = (id: Id.t, ed: Editor.t, eds: t): t =>
  switch (eds) {
  | DebugLoad => failwith("no editors in debug load mode")
  | Scratch(n, slides) =>
    assert(n < List.length(slides));
    let slide = List.nth(slides, n);
    Scratch(
      n,
      Util.ListUtil.put_nth(
        n,
        ScratchSlide.put_editor_and_id(slide, id, ed),
        slides,
      ),
    );
  | School(n, specs, exercise) =>
    School(n, specs, SchoolExercise.put_editor_and_id(exercise, id, ed))
  };

let active_zipper = (editors: t): Zipper.t =>
  get_editor(editors).state.zipper;

let seg_for_semantics = (ed: Editor.t): Segment.t =>
  Zipper.smart_seg(~erase_buffer=true, ~dump_backpack=true, ed.state.zipper);

let export_ctx = (init_ctx: Ctx.t, ed: Editor.t): Ctx.t => {
  let stdlib_seg = seg_for_semantics(ed);
  let (term, _) = MakeTerm.go(stdlib_seg);
  let info_map = Statics.mk_map_ctx(init_ctx, term);
  switch (Id.Map.find_opt(Hyper.export_id, info_map)) {
  | None => Ctx.empty
  | Some(info) => Info.ctx_of(info)
  };
};

let export_env = (init_env: Environment.t, ed: Editor.t) => {
  let tests =
    seg_for_semantics(ed)
    |> Interface.eval_segment_to_result(init_env)
    |> ProgramResult.get_state
    |> EvaluatorState.get_tests
    |> TestMap.lookup(Hyper.export_id);
  switch (tests) {
  | Some([(_, _, env), ..._]) => env
  | Some([])
  | None => Environment.empty
  };
};

let deps = (fn: ('a, 'b) => 'a, acc_0: 'a, slides, idx) => {
  let get = idx => List.nth(slides, idx) |> snd;
  let acc_1 = 1 |> get |> fn(acc_0);
  let acc_2 = 2 |> get |> fn(acc_1);
  let acc_3 = 3 |> get |> fn(acc_2);
  let acc_4 = 4 |> get |> fn(acc_3);
  switch (idx) {
  | 0
  | 1 => acc_0
  | 2 => acc_1
  | 3 => acc_2
  | 4 => acc_3
  | _ => acc_4
  };
};

let get_ctx_init_slides =
  deps(export_ctx, Builtins.ctx(Builtins.Pervasives.builtins));
let get_env_init_slides = deps(export_env, Environment.empty);

let get_ctx_init = (editors: t): Ctx.t =>
  switch (editors) {
  | DebugLoad => Ctx.empty
  | Scratch(idx, slides) => get_ctx_init_slides(slides, idx)
  | School(_, _, _) => Ctx.empty
  };

let get_spliced_elabs =
    (editors: t): list((ModelResults.key, DHExp.t, Environment.t)) => {
  switch (editors) {
  | DebugLoad => []
  | Scratch(idx, slides) =>
    //let ed: Editor.t = List.nth(slides, Hyper.export_slide) |> snd;
    //let env = export_env(Environment.empty, ed);
    let current_slide = List.nth(slides, idx);
    ScratchSlide.spliced_elabs(
      ~ctx_init=get_ctx_init_slides(slides, idx),
      current_slide,
    )
    |> List.mapi((idx, (key, d)) =>
         (key, d, get_env_init_slides(slides, idx))
       );
  | School(_, _, exercise) => SchoolExercise.spliced_elabs(exercise)
  };
};

let set_instructor_mode = (editors: t, instructor_mode: bool): t =>
  switch (editors) {
  | DebugLoad => failwith("no editors in debug load mode")
  | Scratch(_) => editors
  | School(n, specs, exercise) =>
    School(
      n,
      specs,
      SchoolExercise.set_instructor_mode(exercise, instructor_mode),
    )
  };

let num_slides = (editors: t): int =>
  switch (editors) {
  | DebugLoad => 0
  | Scratch(_, slides) => List.length(slides)
  | School(_, specs, _) => List.length(specs)
  };

let cur_slide = (editors: t): int =>
  switch (editors) {
  | DebugLoad => 0
  | Scratch(n, _)
  | School(n, _, _) => n
  };
