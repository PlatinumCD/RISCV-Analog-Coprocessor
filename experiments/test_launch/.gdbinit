#set pagination off
set breakpoint pending on
set logging file gdb.output
set logging enabled on 

break SST::Vanadis::VanadisDebugComponent::tick
set $cami = 0
command 1
	silent

	if(cycle % 100 == 0)

		if($cami == 0)
			printf "Cycle [%d]: ", cycle
		end
		printf "%d ", this->core_id
		set $cami = $cami + 1
		if($cami == 3)
			printf "\n"
			set $cami = 0
		end
	end
	continue
end

disable 1

break SST::Vanadis::VanadisCloneSyscall::VanadisCloneSyscall
command 2
#	continue
end

#break SST::Golem::MVMComputeArray::compute
#command 3
#	enable 1
#end

run
