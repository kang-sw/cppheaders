
/**
 * usage:
 * \code
 *   std::string buffer;
 *
 *   // forward args for construct json_serializer
 *   basic_builder<json_serializer> obj(&buffer, ...);
 *
 *   // with simple assignment, it'll replace current context.
 *   // (key-index context/ value context)
 *   obj = 4;              // 4
 *   // obj = "hell, world"; // error: invalid context
 *
 *   obj.clear();          //
 *   obj = "hell, world!"; // "hell, world!"
 *
 *   obj.clear();
 *
 *   // with index operator, it creates new objects
 *   // since current context is not object, it'll reset.
 *   obj["a"];     // {"a":
 *   obj["b"];     // {"a": {"b":
 *   obj = 3;      // {"a": {"b": 3
 *
 *   obj["c"];     // {"a": {"b": 3, "c":
 *   obj = "cc";   // {"a": {"b": 3, "c": "cc"
 *
 *   obj.break();  // {"a": {"b": 3, "c": "cc"}
 *
 *   obj["d"];     // {"a": {"b": 3, "c": "cc"}, "d":
 *
 *   // obj.break(); // error: invalid builder context

 *   obj = nullptr;  // {"a": {"b": 3, "c": "cc"}, "d": null
 *   obj.break();    // {"a": {"b": 3, "c": "cc"}, "d": null}
 *
 *   obj.clear();
 *
 *   obj[0];         // [
 *   obj = 1;        // [1
 *   obj[2];         // [1, null,
 *
 *   // obj[1]; // error: cannot invert index order
 *   obj = "ola";    // [1, null, "ola"
 *   obj["brk"];     // [1, null, "ola", {"brk":
 *   obj = nullptr;  // [1, null, "ola", {"brk": null
 *
 *   obj.break().break(); // [1, null, "ola", {"brk": null}]
 * \endcode
 */