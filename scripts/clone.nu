let repos = (gh repo list bazelboost -L 150 --json name | from json)

for $r in $repos {
  let name = ($r | get name)
  if not ($name | path exists) {
    try {
      gh repo clone ('bazelboost/' + $name)
    } catch {|e|
      print $e
    }
  }
}