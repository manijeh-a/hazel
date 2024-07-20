open Util;
open Sexplib.Std;
open Ppx_yojson_conv_lib.Yojson_conv.Primitives;
open Virtual_dom.Vdom;
open ProjectorBase;

let of_id = (id: Id.t) =>
  "id" ++ (id |> Id.to_string |> String.sub(_, 0, 8));

let escape_linebreaks: string => string =
  Re.Str.global_replace(Re.Str.regexp("\n"), "\\n");

let unescape_linebreaks: string => string =
  Re.Str.global_replace(Re.Str.regexp("\\\\n"), "\n");

let of_mono = (syntax: Piece.t): option(string) =>
  switch (syntax) {
  | Tile({label: [l], _}) => Some(unescape_linebreaks(l))
  | _ => None
  };

let mk_mono = (sort: Sort.t, string: string): Piece.t =>
  string |> escape_linebreaks |> Form.mk_atomic(sort) |> Piece.mk_tile(_, []);

let get = (piece: Piece.t): string =>
  switch (piece |> of_mono) {
  | None => failwith("TextArea: not string literal")
  | Some(s) => s
  };

let put = (s: string): Piece.t => s |> mk_mono(Exp);

let put = (str: string): ProjectorBase.action =>
  SetSyntax(str |> Form.string_quote |> put);

let key_handler = (id, ~inject, evt) => {
  open Effect;
  let key = Key.mk(KeyDown, evt);
  let pos = JsUtil.TextArea.caret_rel_pos(JsUtil.TextArea.get(of_id(id)));
  let is_last_pos = pos.rows == Last && pos.cols == Last;
  let is_first_pos = pos.rows == First && pos.cols == First;
  switch (key.key) {
  | D("ArrowRight" | "ArrowDown") when is_last_pos =>
    JsUtil.get_elem_by_id(of_id(id))##blur;
    Many([inject(Escape(Right)), Prevent_default, Stop_propagation]);
  | D("ArrowLeft" | "ArrowUp") when is_first_pos =>
    JsUtil.get_elem_by_id(of_id(id))##blur;
    Many([inject(Escape(Left)), Prevent_default, Stop_propagation]);
  | _ => Stop_propagation
  };
};

let textarea =
    (id, ~inject: ProjectorBase.action => Ui_effect.t(unit), text: string) =>
  Node.textarea(
    ~attrs=[
      Attr.id(of_id(id)),
      Attr.on_keydown(key_handler(id, ~inject)),
      Attr.on_input((_, new_text) => inject(put(new_text))),
    ],
    [Node.text(text)],
  );

let n_of = (n: int) =>
  //·•⬤
  [Node.text("·")]
  @ (List.init(n, _ => [Node.br(), Node.text("·")]) |> List.flatten);

let view = (_, ~info, ~go as _, ~inject) => {
  let text = info.syntax |> get |> Form.strip_quotes;
  Node.div(
    ~attrs=[Attr.classes(["cols"])],
    n_of(Util.StringUtil.num_linebreaks(text))
    @ [textarea(info.id, ~inject, text)],
  );
};

let line_lengths = syntax =>
  List.map(String.length, Re.Str.split(Re.Str.regexp("\n"), get(syntax)));

let num_lines = syntax => List.fold_left(max, 0, line_lengths(syntax));

let placeholder = (_, info) =>
  Block({
    row: Util.StringUtil.num_linebreaks(get(info.syntax)),
    col: 2 + num_lines(info.syntax) /* +2 for left and right padding */
  });

module M: Projector = {
  [@deriving (show({with_path: false}), sexp, yojson)]
  type model = unit;
  [@deriving (show({with_path: false}), sexp, yojson)]
  type action = unit;
  let init = ();
  let can_project = _ => true; //TODO(andrew): restrict somehow
  let placeholder = placeholder;
  let update = (model, _) => model;
  let view = view;
  let activate = ((id: Id.t, d: Direction.t)) => {
    JsUtil.get_elem_by_id(of_id(id))##focus;
    switch (d) {
    | Left => ()
    | Right =>
      JsUtil.TextArea.set_caret_to_end(JsUtil.TextArea.get(of_id(id)))
    };
  };
};
