1 Replace /∂-∂-∂- (≈)®1∂t≈/ ®1 ; Set FILE1 . ; Print "File #1: " . ; Next
2 Replace /∂+∂+∂+ (≈)®1∂t≈/ ®1 ; Set FILE2 . ; Print "File #2: " . "\n" ; Next

# a b
/@@ ∂-([0-9]+)®1 ∂+([0-9]+)®3 @@/
	Print "### Nonmatching lines"
	Print "	Find " ®1 " \"" FILE1 "\"; Find " ®3 " \"" FILE2 "\""
	Next

# a b,0
/@@ ∂-([0-9]+)®1 ∂+([0-9]+)®3,0 @@/
	Print "### Extra lines in 1st after " ®3 " in 2nd"
	Print "	Find " ®1 " \"" FILE1 "\"; Find " ®3 "∆ \"" FILE2 "\""
	Next

# a,n b,0
/@@ ∂-([0-9]+)®1,([1-9][0-9]*)®2 ∂+([0-9]+)®3,0 @@/
	Print "### Extra lines in 1st after " ®3 " in 2nd"
	Print "	Find " ®1 ":∆(!" ®2 ") \"" FILE1 "\"; Find " ®3 "∆ \"" FILE2 "\""
	Next

# a,0 b
/@@ ∂-([0-9]+)®1,0 ∂+([0-9]+)®3 @@/
	Print "### Extra lines in 2nd after " ®1 " in 1st"
	Print "	Find " ®1 "∆ \"" FILE1 "\"; Find " ®3 " \"" FILE2 "\""
	Next

# a,0 b,n
/@@ ∂-([0-9]+)®1,0 ∂+([0-9]+)®3,([1-9][0-9]*)®4 @@/
	Print "### Extra lines in 2nd after " ®1 " in 1st"
	Print "	Find " ®1 "∆ \"" FILE1 "\"; Find " ®3 ":∆(!" ®4 ") \"" FILE2 "\""
	Next

# a b,n
/@@ ∂-([0-9]+)®1 ∂+([0-9]+)®3,([1-9][0-9]*)®4 @@/
	Print "### Nonmatching lines"
	Print "	Find " ®1 " \"" FILE1 "\"; Find " ®3 ":∆(!" ®4 ") \"" FILE2 "\""
	Next

# a,n b
/@@ ∂-([0-9]+)®1,([1-9][0-9]*)®2 ∂+([0-9]+)®3 @@/
	Print "### Nonmatching lines"
	Print "	Find " ®1 ":∆(!" ®2 ") \"" FILE1 "\"; Find " ®3 " \"" FILE2 "\""
	Next

# a,n b,n
/@@ ∂-([0-9]+)®1,([0-9]*)®2 ∂+([0-9]+)®3,([0-9]*)®4 @@/
	Print "### Nonmatching lines"
	Print "	Find ∆" ®1 ":∆(!" ®2 ") \"" FILE1 "\"; Find ∆" ®3 ":∆(!" ®4 ") \"" FILE2 "\""
	Next
