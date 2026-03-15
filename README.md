# Teleos Language

**Extension:** `.tsl` / `.tslc` (compiled)
**Compiler:** g++ (C++17)
**Version:** 1.0

Teleos is built to be the hardest yet most readable language ever made.

Changelogs

- Added cpu_usage : %
- Added ram_usage : %
- Do not use the current fetch, it doesn't work fall back to the community modules. (Or use os.run)

---

## Installing (Admin Auto-Prompted)

Double-click `INSTALL_ME.bat`. It will UAC-prompt for admin automatically, build `teleos.exe`, copy it to `C:\teleos\`, and add it to your system PATH.

After it finishes, **restart your terminal or VS Code**, then:

```
teleos yourscript.tsl
```

To uninstall:
```
uninstall.bat
```

---

## Building Manually

### Windows
```
build.bat
```

### Linux / macOS
```
chmod +x build.sh
./build.sh
```

---

## Running a Script

```
teleos script.tsl
```

---

## Compile & Obfuscate

Protect your source code by compiling `.tsl` files into unreadable `.tslc` binaries. The compiled file runs identically — source is stripped of comments, XOR-encrypted, and base64-encoded with an embedded random key.

```
teleos compile myscript.tsl
```

Output: `myscript.tslc` — completely unreadable.

```
teleos myscript.tslc
```

Runs identically to the original. Auto-detected as compiled.

> ⚠️ **Never run `.tslc` files from untrusted sources.** Compiled files hide their source — you cannot inspect what they will do before running.

---

## Language Reference

---

### Comments

```
` This is a comment. `
```

### Paragraphs

```
```.

This is a paragraph block.
It can span multiple lines.

```,
```

---

### Variables

```
create var = var1 > "Hello, this is a string variable."
create var = var2 > 100
create var = var3 > {["This is a table."], ["Yes."]}
create var = var4 > true ` can be false too. `
```

### Constants

```
const var = PI > 3.14159
const var = APP_NAME > "Teleos"
```

Constants cannot be reassigned after declaration.

### Variable Reassignment

```
create var = x > 10
x = 99
console.output(x)
```

### Increment / Decrement

```
x++
x--
```

---

### Output / Printing

```
` simple `
console.output("Hi")
console.output(var1)

` no newline `
console.print("Loading...")
console.println(" Done!")

` inside a call block `
call function; true then
 do console.output("This is an output.")
 end
function.end(function;true[1]) ` auto-detect if first function `

` variable calling `
call variable; var = var1 then
 do console.output("This is an output.")
 end
function.end(variable;var=var1[1])
```

### Input

```
console.input("Enter your name: ")(username)
console.output(username)
```

---

### End Blocks

```
function.end(myFunction;)
end
```

---

### Console Utilities

```
console.clear()
console.color("red")
console.color("green")
console.color("blue")
console.color("yellow")
console.color("cyan")
console.color("white")
console.color("reset")
console.title("My Teleos App")
console.wait(2000) ` wait 2000 milliseconds `
```

---

### If Statements

```
do if variable; var = var1 == "Hello" then
 do console.output("Matched!")
end

do if variable; var = var1 == true then
 do console.output("It is true!")
end
```

---

### Why Statements (Custom)

```
function call; why then do
 why var1 == "Hello" then
  console.output("Why? Because it is Hello.")
 end
end
```

---

### Unless Statements

```
function call; unless then do
 unless var1 == "Hi" then
  console.output("It is not Hi!")
 end
end
```

---

### Calculation

```
math.call; 1+1 = output ` output is a variable. `
 console.output(output)
end

math.call; 2^10 = result ` exponent operator `
 console.output(result)
end
```

Supports: `+` `-` `*` `/` `%` `^` (exponent)

---

### Math Functions

```
math.floor(3.9) = fl
math.ceil(3.1) = cl
math.sqrt(16) = sq
math.abs(-42) = ab
math.pow(2, 8) = pw
math.min(10, 20) = mn
math.max(10, 20) = mx
math.round(3.6) = rd
math.sin(1.5) = sn
math.cos(0) = cs
math.tan(0) = tn
math.log(2.71828) = lg
math.mod(10, 3) = md
math.random(1, 100) = rnd
console.output(rnd)
```

---

### String Functions

```
string.len("Hello") = l
string.upper("hello") = u
string.lower("HELLO") = lo
string.trim("  hello  ") = t
string.split("a,b,c", ",") = parts
string.join(parts, " | ") = joined
string.contains("Hello World", "World") = found
string.replace("Hello World", "World", "Teleos") = replaced
string.starts("Teleos", "Tel") = sw
string.ends("Teleos", "eos") = ew
string.sub("Hello World", 6, 5) = sub
string.find("Hello World", "World") = pos
string.repeat("ha", 3) = rep
string.reverse("Teleos") = rev
string.format("{} is version {}", "Teleos", "1.0") = formatted
```

---

### Table Functions

```
create var = t > {["apple"], ["banana"], ["cherry"]}

table.push(t, "mango")
table.pop(t) = last
table.len(t) = tlen
table.get(t, 0) = first
table.set(t, 1, "blueberry")
table.remove(t, 0)
table.contains(t, "cherry") = has
table.keys(t) = keys
table.merge(t, {["extra"]}) = merged
table.sort(t) = sorted
table.slice(t, 0, 2) = sliced
```

---

### Functions (define / return)

```
define greet(name) then
 return string.format("Hello, {}!", name)
end

define add(a, b) then
 return a + b
end

define factorial(n) then
 do if n == 0 then
  do return 1
 end
 return n * factorial(n - 1)
end

create var = msg > greet("World")
console.output(msg)
create var = result > factorial(6)
console.output(result)
```

---

### Repeat Loops

```
repeat var1 until == 1000 num then
 console.output("Yo")
end

repeat var1 until == nil num then ` nil = infinite `
 console.output("Yo")
 repeat(finish) ` stop the loop `
end
```

---

### While Loops

```
create var = count > 0
while count < 10 then
 console.output(count)
 count++
end

` break out early `
create var = i > 0
while i < 100 then
 do if i == 5 then
  do break
 end
 i++
end
console.output(i) ` 5 `

` decrement `
create var = x > 10
while x > 0 then
 x--
end
console.output(x) ` 0 `
```

---

### Range

```
range.call; 1 to 10 ; nums
console.output(nums)
```

---

### Switch

```
create var = color > "blue"

switch(color) then
 case "red" then
  console.output("Red!")
 end
 case "blue" then
  console.output("Blue!")
 end
 default then
  console.output("Other color.")
 end
end
```

---

### Try / Catch / Finally

```
try then
 throw "Something broke"
catch (err) then
 console.output(err)
finally then
 console.output("Always runs.")
end
```

---

### Enum

```
enum Direction then
 NORTH, SOUTH, EAST, WEST
end

console.output(NORTH) ` 0 `
console.output(EAST)  ` 2 `

create var = heading > EAST
do if heading == EAST then
 do console.output("Going East!")
end
```

---

### Assert & Inspect

```
` assert halts with an error if condition is false `
assert 1 == 1, "Math is broken"
assert score > 50, "Score too low!"
console.output("All assertions passed!")

` inspect prints type and value for debugging `
create var = n > 42
inspect(n) ` <inspect type=number value=42> `

create var = t > {["a"], ["b"]}
inspect(t) ` <inspect type=table[2] value=...> `
```

---

### Type System

```
type.of(42) = t
console.output(t) ` number `

type.is("hello", "string") = check
console.output(check) ` true `

type.cast(99, "string") = s
console.output(s) ` "99" `
```

---

### File I/O

```
file.write("out.txt", "Hello!")
file.append("out.txt", "\nMore text")
file.read("out.txt") = contents
file.exists("out.txt") = exists
file.lines("out.txt") = lines
file.size("out.txt") = sz
file.copy("out.txt", "copy.txt")
file.move("copy.txt", "moved.txt")
file.delete("moved.txt")
file.mkdir("myfolder")
```

---

### Hardware Logging and Configuration

```
system.hardware;call;find = "hardware", "cpu" = variable1
system.hardware;call;find = "hardware", "vram" = variable1
system.hardware;call;find = "hardware", "storage" = variable1
system.hardware;call;find = "hardware", "gpu" = variable1
system.hardware;call;find = "hardware", "ram" = variable1
system.hardware;call;find = "hardware", "global" = variable1
console.output(variable1)
```

---

### Application Opening

```
local system function call
 system.hardware do
  app.open = ["Roblox.exe"]
 end
end

local system function call
 system.hardware do
  app.open = ["C:\Roblox.exe"]
 end
end
```

---

### Logging to File

```
calllog{[("Logged")]}.to == "C:\text.txt"
calllog{[("Logged")]}.to == "text.txt"
```

Logs are timestamped automatically.

---

### OS Functions

```
os.platform = plat     ` "windows", "linux", or "macos" `
os.cwd = cwd           ` current working directory `
os.date = d            ` "2026-03-15 12:00:00" `
os.time = t            ` unix timestamp `
os.env("PATH") = p     ` read environment variable `
os.sleep(1000)         ` sleep 1000ms `
os.run("echo hello")   ` run a shell command `
os.exit(0)             ` exit with code `
os.args = args         ` command line arguments as table `
```

---

### Comparing

```
function compare(variable1) to (variable2) and then
 if function.compare.current(variable1) == "LOL" then
  console.output("Hello")
  end
 end
end

function compare(variable1) to (variable2) and then
 if function.compare.current(variable1) < 50 then
  console.output("Hello")
  end
 end
end
```

---

### Asking

```
console.ask("How are you?")["Good", "Bad"](variableask)

console.check.variableask(ask) then
 if variableask == "Good" then
  console.output("Glad to hear it!")
  end
 end
end
```

---

### Modules

```
` mymodule.tsl `
export module(global)

create var = varmodule > "Hello from module!"

` main.tsl `
import "mymodule" ` or "mymodule.tsl" `
console.output(varmodule)
```

---

## Custom Errors

| Error | When it fires |
|---|---|
| `SyntaxError` | Malformed syntax |
| `UnknownStatement` | Unrecognized keyword or statement |
| `UndefinedVariable` | Accessing a variable that was never declared |
| `TypeMismatch` | Wrong types used in an operation |
| `DivisionByZero` | Dividing or modding by zero |
| `ModuleNotFound` | Imported module file doesn't exist |
| `FileIOError` | Cannot open or write a file |
| `HardwareQueryError` | Hardware query call failed |
| `AppLaunchError` | Application failed to launch |
| `FetchError` | HTTP request failed |
| `RuntimeError` | General runtime failure |
| `OverflowError` | Numeric overflow |
| `ImportError` | Module load failure |
| `IndexOutOfRange` | Table or string index out of bounds |
| `InvalidArgument` | Bad argument passed to a built-in |

---

## Examples

| File | What it shows |
|---|---|
| `hello.tsl` | Hello World |
| `variables.tsl` | All variable types |
| `ifstatements.tsl` | If statements |
| `loops.tsl` | Repeat loops |
| `while_and_control.tsl` | While loops, break, ++/-- |
| `math.tsl` | Math operations |
| `unless_why.tsl` | Unless and Why statements |
| `functions.tsl` | define / return / recursion |
| `strings.tsl` | All string functions |
| `tables.tsl` | All table functions |
| `fileio.tsl` | File read/write/copy/delete |
| `trycatch.tsl` | Try / catch / finally |
| `switch.tsl` | Switch / case / default |
| `enum.tsl` | Enums |
| `assert_inspect.tsl` | Assert and inspect |
| `console_print.tsl` | console.print / console.println |
| `osinfo.tsl` | OS queries and math functions |
| `range_and_const.tsl` | range.call and const |
| `typecheck.tsl` | type.of / type.is / type.cast |
| `compile_usage.tsl` | Compile / obfuscate usage |
| `compile_demo.tsl` | Compile demo script |
| `mymodule.tsl` | Module definition |
| `importdemo.tsl` | Module import (run from examples/) |
| `fulldemo.tsl` | Everything combined |
