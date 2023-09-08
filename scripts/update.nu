cd ../
let repos = (ls | where type == dir)
let cwd = $env.PWD

for r in $repos {
  let name = ($r.name | path basename)

  if $name == 'registry' { continue }
  if $name == 'boost' { continue }
  if $name == '_bazel' { continue }
  if $name == 'bzlboostgen' { continue }

  if $name == 'config' { continue } # temp

  
  print ('  === ' + $name + ' === ')
  
  cd ([$cwd, $name] | path join)

  try {
    # git fetch --all --tags --force
    # git checkout boost-1.83.0
    # try { git checkout -b bazelboost-1.83.0 } catch {
    #   git checkout bazelboost-1.83.0
    # }
    # try { bzlboostgen } catch {}
    # git push origin -u bazelboost-1.83.0
    # gh repo set-default ('bazelboost/' + $name)

    # git clean -fdx
    # bzlboostgen

    # git clean -fdx
    try {
      bzlboostgen -registry=../registry -module_version=1.83.0.bzl.1 -skip_tests

      if (( git status --porcelain | complete ).stdout | is-empty) {
        continue
      }

      bazel build ...

      git add .github .bazelrc .gitignore *.bazel
      git commit -m "feat: initial bzlmod support"
      git push origin
      gh release create bazelboost-1.83.0.bzl.1 -t "bazelboost-1.83.0.bzl.1" --generate-notes
    } catch {
      # undo changes if we failed
      git clean -fdx
    }

  } catch {}
}