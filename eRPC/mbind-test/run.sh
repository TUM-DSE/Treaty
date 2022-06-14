# A function to echo in blue color
function blue() {
	es=`tput setaf 4`
	ee=`tput sgr0`
	echo "${es}$1${ee}"
}


# Test if NUMA-node binding of shmat regions works */
for preferred_node in 0 1; do
	sudo ipcrm -M 3185

	blue "Running mbind-test: allocating hugepages on node $preferred_node"
	sudo ./mbind-test $preferred_node

	blue "Printing used hugepages"
	hugepages-check.sh
done
