open Sexplib.Std;
open Ppx_yojson_conv_lib.Yojson_conv.Primitives;
open Virtual_dom.Vdom;
open ProjectorBase;

/* Some decimal places necessary to avoid becoming an int */
let float_of_float = s => s |> float_of_string |> Printf.sprintf("%.2f");

let put = (s: string): Piece.t => s |> float_of_float |> Piece.mk_mono(Exp);

let get_opt = (piece: Piece.t): option(float) =>
  piece |> Piece.of_mono |> Util.OptUtil.and_then(float_of_string_opt);

let get = (piece: Piece.t): float =>
  switch (get_opt(piece)) {
  | None => failwith("ERROR: Slider: not float literal")
  | Some(s) => s
  };

module M: CoreInner = {
  [@deriving (show({with_path: false}), sexp, yojson)]
  type model = unit;
  let init = ();
  let can_project = p => get_opt(p) != None;
  let placeholder = (_, _) => Inline(10);
  let update = (model, _) => model;
  let view = (_, ~info, ~inject: ProjectorBase.action => Ui_effect.t(unit)) =>
    Util.Web.range(
      ~attrs=[Attr.on_input((_, v) => inject(SetSyntax(put(v))))],
      get(info.syntax) |> Printf.sprintf("%.2f"),
    );
  let activate = _ => ();
  // let keymap = (_, _, _) => None;
};
