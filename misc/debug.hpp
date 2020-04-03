#pragma once
#include <string>
#include <set>
#include <platform.hpp>
#include "..\arch\instruction_set.hpp"
#include "..\routine\basic_block.hpp"
#include "..\routine\instruction.hpp"
#include "format.hpp"

namespace vtil::debug
{
	void dump( const instruction& ins, const instruction* prev = nullptr )
	{
		// Print stack pointer offset
		//
		if ( ins.sp_reset )
			io::log<CON_PRP>( ">%c0x%-4x ", ins.sp_offset >= 0 ? '+' : '-', abs( ins.sp_offset ) );
		else if ( ( prev ? prev->sp_offset : 0 ) == ins.sp_offset )
			io::log<CON_DEF>( "%c0x%-4x  ", ins.sp_offset >= 0 ? '+' : '-', abs( ins.sp_offset ) );
		else if ( ( prev ? prev->sp_offset : 0 ) > ins.sp_offset )
			io::log<CON_RED>( "%c0x%-4x  ", ins.sp_offset >= 0 ? '+' : '-', abs( ins.sp_offset ) );
		else
			io::log<CON_BLU>( "%c0x%-4x  ", ins.sp_offset >= 0 ? '+' : '-', abs( ins.sp_offset ) );

		// Print name
		//
		if ( ins.is_volatile() )
			io::log<CON_RED>( FMT_INS_MNM " ", ins.base->to_string( ins.access_size() ) );	// Volatile instruction
		else
			io::log<CON_BRG>( FMT_INS_MNM " ", ins.base->to_string( ins.access_size() ) );	// Non-volatile instruction

		// Print each operand
		//
		for ( auto& op : ins.operands )
		{
			if ( op.is_register() )
			{
				if ( op.reg.base.maps_to == X86_REG_RSP )
					io::log<CON_PRP>( FMT_INS_OPR " ", op.reg.to_string() );				// Stack pointer
				else if ( op.reg.base.maps_to != X86_REG_INVALID )
					io::log<CON_BLU>( FMT_INS_OPR " ", op.reg.to_string() );				// Any hardware/special register
				else
					io::log<CON_GRN>( FMT_INS_OPR " ", op.reg.to_string() );				// Virtual register
			}
			else
			{
				fassert( op.is_immediate() );

				if ( ins.base->memory_operand_index  != -1 &&
					 &ins.operands[ ins.base->memory_operand_index + 1 ] == &op &&
					 ins.operands[ ins.base->memory_operand_index ].reg == X86_REG_RSP )
				{
					if ( op.i64 >= 0 )
						io::log<CON_YLW>( FMT_INS_OPR " ", format::hex( op.i64 ) );			 // External stack
					else
						io::log<CON_BRG>( FMT_INS_OPR " ", format::hex( op.i64 ) );			 // VM stack
				}
				else
				{
					io::log<CON_CYN>( FMT_INS_OPR " ", format::hex( op.i64 ) );				 // Any immediate
				}
			}
		}

		// Print padding and end line
		//
		fassert( ins.operands.size() <= arch::max_operand_count );
		for ( int i = ins.operands.size(); i < arch::max_operand_count; i++ )
			io::log( FMT_INS_OPR " ", "" );
		io::log( "\n" );
	}

	void dump( const basic_block* blk, std::set<const basic_block*>* visited = nullptr )
	{
		bool blk_visited = visited ? visited->find( blk ) != visited->end() : false;

		auto end_with_bool = [ ] ( bool b )
		{
			if ( b ) io::log<CON_GRN>( "Y\n" );
			else io::log<CON_RED>( "N\n" );
		};

		io::log<CON_DEF>( "Entry point VIP:       " );
		io::log<CON_CYN>( "0x%llx\n", blk->entry_vip );
		io::log<CON_DEF>( "Stack pointer:         " );
		if ( blk->sp_offset < 0 )
			io::log<CON_RED>( "%s\n", format::hex( blk->sp_offset ) );
		else
			io::log<CON_GRN>( "%s\n", format::hex( blk->sp_offset ) );
		io::log<CON_DEF>( "Already visited?:      " ); 
		end_with_bool( blk_visited );
		io::log<CON_DEF>( "------------------------\n" );

		if ( blk_visited )
			return;

		// Print each instruction
		//
		int ins_idx = 0;
		for ( auto it = blk->begin(); it != blk->end(); it++, ins_idx++ )
		{
			io::log<CON_BLU>( "%04d: ", ins_idx );
			if ( it->vip == invalid_vip )
				io::log<CON_DEF>( "[PSEUDO] " );
			else
				io::log<CON_DEF>( "[%06x] ", it->vip );
			dump( *it, it.is_begin() ? nullptr : &*std::prev( it ) );
		}

		// Dump each branch as well
		//
		if ( visited )
		{
			visited->insert( blk );
			io::log_padding++;
			io::log( "\n" );
			for ( auto& child : blk->next )
				dump( child, visited );
			io::log_padding--;
			io::log( "\n" );
		}
	}

	void dump( const routine* routine )
	{
		std::set<const basic_block*> vs;
		dump( routine->entry_point, &vs );
	}
};